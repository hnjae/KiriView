// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "localization/imageerrortext.h"
#include "location/imageurl.h"
#include "navigation/mediaformatregistry.h"
#include "navigation/navigationlogging.h"
#include "session/documentsessiondirectmedianavigationworkflow.h"

#include <QAbstractListModel>
#include <QDebug>
#include <QObject>
#include <QScopedValueRollback>
#include <QString>
#include <utility>

namespace {
QString genericFileDeletionErrorMessage()
{
    return kiriview::imageErrorText(kiriview::ImageErrorTextId::DeleteFile);
}

const char *documentKindName(kiriview::DocumentSessionKind kind)
{
    switch (kind) {
    case kiriview::DocumentSessionKind::Empty:
        return "Empty";
    case kiriview::DocumentSessionKind::Image:
        return "Image";
    case kiriview::DocumentSessionKind::Video:
        return "Video";
    }

    return "Unknown";
}

const char *routeKindName(kiriview::DocumentSessionRouteKind kind)
{
    switch (kind) {
    case kiriview::DocumentSessionRouteKind::Empty:
        return "Empty";
    case kiriview::DocumentSessionRouteKind::DirectVideo:
        return "DirectVideo";
    case kiriview::DocumentSessionRouteKind::DirectImage:
        return "DirectImage";
    case kiriview::DocumentSessionRouteKind::ImageDocument:
        return "ImageDocument";
    }

    return "Unknown";
}

void logDirectMediaScope(const char *message, const kiriview::DirectMediaScope &scope)
{
    qCDebug(kiriviewNavigationLog) << message << "currentUrl" << scope.currentUrl << "parentUrl"
                                   << scope.parentUrl << "generation" << scope.generation;
}

void appendConnection(std::vector<QMetaObject::Connection> &connections,
    const kiriview::DocumentSessionDocumentSignalConnector &connector, QObject *owner,
    kiriview::DocumentSessionDocumentChangeHandler handler)
{
    if (connector) {
        std::vector<QMetaObject::Connection> nextConnections = connector(owner, std::move(handler));
        connections.insert(connections.end(), nextConnections.begin(), nextConnections.end());
    }
}

bool sameActiveNavigationSnapshot(const kiriview::ImageDocumentPageActiveNavigationSnapshot &left,
    const kiriview::ImageDocumentPageActiveNavigationSnapshot &right)
{
    return left.known == right.known && left.canOpenPrevious == right.canOpenPrevious
        && left.canOpenNext == right.canOpenNext && left.atKnownFirst == right.atKnownFirst
        && left.atKnownLast == right.atKnownLast && left.currentNumber == right.currentNumber
        && left.count == right.count;
}

}

namespace kiriview {
DocumentSessionRuntime::DocumentSessionRuntime(QObject *owner,
    DocumentSessionImageDocumentPort imageDocument, DocumentSessionVideoDocumentPort videoDocument,
    ChangeCallback changeCallback, DocumentSessionRuntimeDependencies dependencies)
    : m_owner(owner)
    , m_imageDocument(std::move(imageDocument))
    , m_videoDocument(std::move(videoDocument))
    , m_state(std::move(changeCallback))
    , m_routeRuntime(DocumentSessionRouteRuntimePorts {
          [this]() { cancelMediaOpenWith(); },
          [this]() { m_state.setSessionErrorString(QString()); },
          [this]() { m_directMediaNavigationRuntime.cancel(); },
          [this]() { cancelMediaDeletion(); },
          [this]() { m_state.setDirectMediaNavigation({}, false, {}); },
          [this]() {
              const bool changed = m_state.clearDirectMediaCursor();
              logDirectMediaScope("direct media cursor cleared", m_state.directMediaScope());
              return changed;
          },
          [this](const QUrl &url) {
              const bool changed = m_state.setDirectVideoCursor(url);
              logDirectMediaScope("direct video cursor set", m_state.directMediaScope());
              return changed;
          },
          [this](const QUrl &url) {
              const bool changed = m_state.requestDirectImageCursor(url);
              logDirectMediaScope("direct image cursor requested", m_state.directMediaScope());
              return changed;
          },
          [this](const std::function<void()> &mutation) {
              QScopedValueRollback<bool> routingSource(m_routingSource, true);
              mutation();
          },
          [this]() {
              m_imageDocument.setSourceUrl(QUrl());
              refreshImagePublicSnapshot();
          },
          [this]() {
              leaveVideoMode();
              refreshVideoPublicSnapshot();
          },
          [this]() { setDocumentKind(DocumentSessionKind::Empty); },
          [this](const QUrl &url) {
              m_imageDocument.setSourceUrl(url);
              refreshImagePublicSnapshot();
              setDocumentKind(DocumentSessionKind::Image);
          },
          [this](const QUrl &url) {
              m_videoDocument.setSourceUrl(url);
              refreshVideoPublicSnapshot();
              setDocumentKind(DocumentSessionKind::Video);
          },
          [this]() {
              const bool changed = syncDirectImageCursorFromDocument();
              logDirectMediaScope(
                  "direct image cursor synced from document", m_state.directMediaScope());
              return changed;
          },
          [this]() { m_state.setSourceIdentity(QUrl()); },
          [this](const QUrl &url) { m_state.setSourceIdentity(url); },
          [this]() { m_state.setSourceIdentity(m_imagePublicSnapshot.sourceUrl); },
          [this]() { recomputePublicProjection(); },
          [this]() { return directMediaNavigationActive(); },
          [this]() { refreshDirectMediaNavigation(); },
          [this]() { m_mediaPredecodeRuntime.clear(); },
          [this]() { clearActiveNavigationRevealContextIfUnavailable(); },
      })
    , m_activeNavigationRuntime(DocumentSessionActiveNavigationRuntimePorts {
          [this](ActiveNavigationRevealContext context) {
              applyActiveNavigationRevealContext(context);
          },
          [this]() { recomputePublicProjection(); },
          [this]() { openPreviousMedia(); },
          [this]() { openNextMedia(); },
          [this](int number) { openMediaAtNumber(number); },
          [this]() { m_imageDocument.openPreviousPage(); },
          [this]() { m_imageDocument.openNextPage(); },
          [this](int number) { m_imageDocument.openImageAtPage(number); },
      })
    , m_activeNavigationThumbnailRuntime(
          owner, &m_imageDocument, std::move(dependencies.activeNavigationThumbnails))
    , m_directMediaNavigationRuntime(dependencies.directMediaNavigationCandidateProvider)
    , m_mediaDeletionRuntime(std::move(dependencies.fileDeletionProvider),
          std::move(dependencies.directMediaNavigationCandidateProvider))
    , m_mediaOpenWithRuntime(std::move(dependencies.mediaOpenWithProvider))
    , m_mediaPredecodeRuntime(owner, std::move(dependencies.directMediaPredecodeDependencies))
{
    refreshLeafPublicSnapshots();
    connectDocuments();
}

DocumentSessionRuntime::~DocumentSessionRuntime()
{
    for (const QMetaObject::Connection &connection : m_documentConnections) {
        QObject::disconnect(connection);
    }
    m_directMediaNavigationRuntime.cancel();
    m_mediaDeletionRuntime.cancel();
    cancelMediaOpenWith();
    m_mediaPredecodeRuntime.cancel();
}

QUrl DocumentSessionRuntime::sourceUrl() const { return m_state.publicSnapshot().sourceUrl; }

void DocumentSessionRuntime::setSourceUrl(const QUrl &sourceUrl) { routeSourceUrl(sourceUrl); }

DocumentSessionKind DocumentSessionRuntime::documentKind() const
{
    return m_state.publicSnapshot().documentKind;
}

quint64 DocumentSessionRuntime::publicProjectionRevision() const
{
    return m_state.publicSnapshot().revision;
}

QString DocumentSessionRuntime::errorString() const { return m_state.publicSnapshot().errorString; }

QString DocumentSessionRuntime::windowTitleSubject() const { return m_state.windowTitleSubject(); }

bool DocumentSessionRuntime::displayedFileDeletionAvailable() const
{
    return m_state.publicSnapshot().projection.displayedFileDeletionAvailable;
}

bool DocumentSessionRuntime::displayedMediaOpenWithAvailable() const
{
    return m_state.publicSnapshot().projection.displayedMediaOpenWithAvailable;
}

bool DocumentSessionRuntime::fileDeletionInProgress() const
{
    return m_state.publicSnapshot().fileDeletionInProgress;
}

const MediaInformationProjectionSnapshot &DocumentSessionRuntime::mediaInformationSnapshot() const
{
    return m_state.mediaInformationSnapshot();
}

bool DocumentSessionRuntime::activeZoomPercentAvailable() const
{
    return m_state.publicSnapshot().activeZoom.available;
}

bool DocumentSessionRuntime::activeZoomPercentKnown() const
{
    return m_state.publicSnapshot().activeZoom.known;
}

qreal DocumentSessionRuntime::activeZoomPercent() const
{
    return m_state.publicSnapshot().activeZoom.percent;
}

bool DocumentSessionRuntime::activeZoomEditable() const
{
    return m_state.publicSnapshot().activeZoom.editable;
}

bool DocumentSessionRuntime::activeImageReady() const
{
    return m_state.publicSnapshot().activeImageReady;
}

bool DocumentSessionRuntime::activeImageUnsupportedOpenedCollectionVideo() const
{
    return m_state.publicSnapshot().activeImageUnsupportedOpenedCollectionVideo;
}

bool DocumentSessionRuntime::activeImageOpenedCollectionScopeActive() const
{
    return m_state.publicSnapshot().activeImageOpenedCollectionScopeActive;
}

bool DocumentSessionRuntime::activeImageRightToLeftReadingActive() const
{
    return m_state.publicSnapshot().activeImageRightToLeftReadingActive;
}

bool DocumentSessionRuntime::activeVideoReady() const
{
    return m_state.publicSnapshot().activeVideoReady;
}

bool DocumentSessionRuntime::activeVideoControlsReady() const
{
    return m_state.publicSnapshot().activeVideoControlsReady;
}

const DocumentSessionActionAvailabilityFacts &
DocumentSessionRuntime::actionAvailabilityFacts() const
{
    return m_state.publicSnapshot().actionAvailability;
}

bool DocumentSessionRuntime::directMediaNavigationActive() const
{
    return m_state.documentKind() == DocumentSessionKind::Video
        || (m_state.documentKind() == DocumentSessionKind::Image
            && directImageLoadMayUseImageDocumentSourceScope());
}

bool DocumentSessionRuntime::activeNavigationAvailable() const
{
    return m_state.activeNavigationSnapshot().available;
}

bool DocumentSessionRuntime::activeNavigationKnown() const
{
    return m_state.activeNavigationSnapshot().known;
}

bool DocumentSessionRuntime::activeNavigationEditable() const
{
    return m_state.activeNavigationSnapshot().editable;
}

bool DocumentSessionRuntime::activeNavigationHasTargets() const
{
    return m_state.activeNavigationSnapshot().count > 0;
}

bool DocumentSessionRuntime::activeNavigationDispatchAvailable() const
{
    const ActiveNavigationSnapshot &snapshot = m_state.activeNavigationSnapshot();
    return snapshot.available && snapshot.known && snapshot.count > 0
        && !m_state.fileDeletionInProgress();
}

int DocumentSessionRuntime::activeNavigationCurrentNumber() const
{
    return m_state.activeNavigationSnapshot().currentNumber;
}

int DocumentSessionRuntime::activeNavigationCount() const
{
    return m_state.activeNavigationSnapshot().count;
}

bool DocumentSessionRuntime::canOpenPreviousActiveNavigation() const
{
    return m_state.activeNavigationSnapshot().canOpenPrevious;
}

bool DocumentSessionRuntime::canOpenNextActiveNavigation() const
{
    return m_state.activeNavigationSnapshot().canOpenNext;
}

bool DocumentSessionRuntime::atKnownFirstActiveNavigation() const
{
    return m_state.activeNavigationSnapshot().atKnownFirst;
}

bool DocumentSessionRuntime::atKnownLastActiveNavigation() const
{
    return m_state.activeNavigationSnapshot().atKnownLast;
}

bool DocumentSessionRuntime::directMediaNavigationBoundaryActive() const
{
    return m_state.activeNavigationBoundaryScope() == ActiveNavigationBoundaryScope::DirectMedia;
}

ActiveNavigationBoundaryScope DocumentSessionRuntime::activeNavigationBoundaryScope() const
{
    return m_state.activeNavigationBoundaryScope();
}

ActiveNavigationRevealIntent DocumentSessionRuntime::activeNavigationRevealIntent() const
{
    return m_state.publicSnapshot().activeNavigationRevealIntent;
}

ActiveNavigationRevealDirection DocumentSessionRuntime::activeNavigationRevealDirection() const
{
    return m_state.publicSnapshot().activeNavigationRevealDirection;
}

QAbstractListModel *DocumentSessionRuntime::activeNavigationThumbnailModel() const
{
    return m_activeNavigationThumbnailRuntime.model();
}

ActiveNavigationThumbnailDemandBucket DocumentSessionRuntime::activeNavigationThumbnailDemandBucket(
    int physicalMaxEdge) const
{
    return m_activeNavigationThumbnailRuntime.demandBucket(physicalMaxEdge);
}

bool DocumentSessionRuntime::reportActiveNavigationThumbnailDemand(int number, const QUrl &url,
    int physicalMaxEdge, ActiveNavigationThumbnailDemandPriority priority,
    quint64 navigationGeneration)
{
    return m_activeNavigationThumbnailRuntime.reportDemand(
        number, url, physicalMaxEdge, priority, navigationGeneration);
}

QString DocumentSessionRuntime::nextVideoOutputSurfaceClaimToken()
{
    return m_videoOutputRuntime.nextSurfaceClaimToken();
}

bool DocumentSessionRuntime::reportVideoOutputSurfaceClaim(const QString &claimToken,
    quint64 projectionRevision, QObject *surfaceOwner, QObject *videoOutput, bool active,
    const QRectF &contentRect, const QRectF &sourceRect)
{
    const bool currentProjection = projectionRevision == m_state.publicSnapshot().revision;
    if (!currentProjection) {
        return false;
    }

    const bool attach = active
        && m_state.publicSnapshot().documentKind == DocumentSessionKind::Video
        && videoOutput != nullptr;

    const DocumentSessionVideoOutputClaimAction action
        = m_videoOutputRuntime.reportSurfaceClaim({ claimToken, surfaceOwner, attach });
    if (action == DocumentSessionVideoOutputClaimAction::Reject) {
        return false;
    }

    if (action == DocumentSessionVideoOutputClaimAction::Detach) {
        m_videoDocument.setVideoOutput(nullptr);
        return true;
    }

    m_videoDocument.setVideoOutput(videoOutput);
    m_videoDocument.setVideoOutputGeometry(contentRect, sourceRect);
    return true;
}

std::optional<PredecodedImage> DocumentSessionRuntime::findPredecodedImage(const QUrl &url) const
{
    return m_mediaPredecodeRuntime.findPredecodedImage(url);
}

void DocumentSessionRuntime::openPreviousMedia()
{
    openMedia(previousDirectMediaNavigationOpenRequest());
}

void DocumentSessionRuntime::openNextMedia() { openMedia(nextDirectMediaNavigationOpenRequest()); }

void DocumentSessionRuntime::openMediaAtNumber(int mediaNumber)
{
    openMedia(numberedDirectMediaNavigationOpenRequest(mediaNumber));
}

void DocumentSessionRuntime::openMedia(DirectMediaNavigationOpenRequest request)
{
    if (!directMediaNavigationActive()) {
        return;
    }

    m_directMediaNavigationRuntime.open(
        m_owner, directMediaNavigationLoadScope(), request,
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        [this](DocumentSessionDirectMediaNavigationOpenResult result) {
            finishDirectMediaNavigation(std::move(result));
        });
}

void DocumentSessionRuntime::applyDirectMediaNavigationRevealAction(
    DocumentSessionDirectMediaNavigationRevealAction action)
{
    switch (action) {
    case DocumentSessionDirectMediaNavigationRevealAction::None:
        return;
    case DocumentSessionDirectMediaNavigationRevealAction::Clear:
        takePendingActiveNavigationRevealContext(ActiveNavigationRevealIntent::None);
        setActiveNavigationRevealContext({});
        return;
    case DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync:
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        return;
    case DocumentSessionDirectMediaNavigationRevealAction::UsePendingOrProgrammaticSync:
        setActiveNavigationRevealContext(takePendingActiveNavigationRevealContext(
            ActiveNavigationRevealIntent::ProgrammaticSync));
        return;
    case DocumentSessionDirectMediaNavigationRevealAction::
        UsePendingOrProgrammaticSyncAndKeepPending: {
        const ActiveNavigationRevealContext context = takePendingActiveNavigationRevealContext(
            ActiveNavigationRevealIntent::ProgrammaticSync);
        setActiveNavigationRevealContext(context);
        setPendingActiveNavigationRevealContext(context);
        return;
    }
    }
}

void DocumentSessionRuntime::openPreviousActiveNavigation() { requestPreviousActiveNavigation(); }

void DocumentSessionRuntime::openNextActiveNavigation() { requestNextActiveNavigation(); }

void DocumentSessionRuntime::openFirstActiveNavigation()
{
    executeActiveNavigationDispatchRequest(firstActiveNavigationDispatchRequest(),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::LargeJump });
}

void DocumentSessionRuntime::openLastActiveNavigation()
{
    executeActiveNavigationDispatchRequest(lastActiveNavigationDispatchRequest(),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::LargeJump });
}

void DocumentSessionRuntime::openActiveNavigationAtNumber(int number)
{
    executeActiveNavigationDispatchRequest(numberedActiveNavigationDispatchRequest(number),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::LargeJump });
}

void DocumentSessionRuntime::openActiveNavigationThumbnailAtNumber(int number)
{
    executeActiveNavigationDispatchRequest(numberedActiveNavigationDispatchRequest(number),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ThumbnailActivation });
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::requestPreviousActiveNavigation()
{
    return executeActiveNavigationDispatchRequest(previousActiveNavigationDispatchRequest(),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::AdjacentNavigation,
            ActiveNavigationRevealDirection::Previous });
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::requestNextActiveNavigation()
{
    return executeActiveNavigationDispatchRequest(nextActiveNavigationDispatchRequest(),
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::AdjacentNavigation,
            ActiveNavigationRevealDirection::Next });
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::executeActiveNavigationDispatchRequest(
    ActiveNavigationDispatchRequest request, ActiveNavigationRevealContext context)
{
    return m_activeNavigationRuntime.dispatch(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(), request, context);
}

void DocumentSessionRuntime::setPendingActiveNavigationRevealContext(
    ActiveNavigationRevealContext context)
{
    m_activeNavigationRuntime.setPendingRevealContext(context);
}

ActiveNavigationRevealContext DocumentSessionRuntime::takePendingActiveNavigationRevealContext(
    ActiveNavigationRevealIntent fallbackIntent)
{
    return m_activeNavigationRuntime.takePendingRevealContext(fallbackIntent);
}

void DocumentSessionRuntime::setActiveNavigationRevealContext(ActiveNavigationRevealContext context)
{
    m_activeNavigationRuntime.setRevealContext(context);
}

void DocumentSessionRuntime::applyActiveNavigationRevealContext(
    ActiveNavigationRevealContext context)
{
    m_state.setActiveNavigationRevealIntent(context.intent);
    m_state.setActiveNavigationRevealDirection(context.direction);
}

void DocumentSessionRuntime::clearActiveNavigationRevealContextIfUnavailable()
{
    m_activeNavigationRuntime.clearRevealContextIfUnavailable(m_state.activeNavigationSnapshot());
}

void DocumentSessionRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_state.documentKind() == DocumentSessionKind::Image
        && !directImageLoadMayUseImageDocumentSourceScope()) {
        m_imageDocument.deleteDisplayedFile(mode);
        syncImageDocumentFileDeletionProgress();
        return;
    }

    if (!displayedFileDeletionAvailable() || !directMediaNavigationActive()) {
        return;
    }

    m_state.setFileDeletionInProgress(true);
    recomputePublicProjection();
    const bool started = m_mediaDeletionRuntime.startForDirectMedia(
        m_owner, mode, directMediaNavigationLoadScope(),
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        m_state.documentKind(),
        [this](DocumentSessionMediaDeletionCompletion completion) {
            finishMediaDeletion(std::move(completion));
        });
    if (!started) {
        m_state.setFileDeletionInProgress(false);
        recomputePublicProjection();
    }
}

void DocumentSessionRuntime::openCurrentMediaWith(MediaOpenWithCallback callback)
{
    m_mediaOpenWithRuntime.open(m_owner, currentMediaOpenWithPlan(), std::move(callback));
}

void DocumentSessionRuntime::connectDocuments()
{
    appendConnection(m_documentConnections, m_imageDocument.snapshotChanged, m_owner,
        [this]() { handleImageDocumentSnapshotChanged(); });
    appendConnection(m_documentConnections, m_videoDocument.snapshotChanged, m_owner,
        [this]() { handleVideoDocumentSnapshotChanged(); });
}

void DocumentSessionRuntime::handleImageDocumentSnapshotChanged()
{
    const ImageDocumentPageActiveNavigationSnapshot previousPageNavigation
        = m_imagePublicSnapshot.pageNavigation;
    refreshImagePublicSnapshot();
    if (m_routingSource || m_state.documentKind() != DocumentSessionKind::Image) {
        return;
    }

    const bool directMediaScopeChanged = syncDirectImageCursorFromDocument();
    m_state.setSourceIdentity(m_imagePublicSnapshot.sourceUrl);
    if (!directImageLoadMayUseImageDocumentSourceScope()) {
        m_state.setFileDeletionInProgress(m_imagePublicSnapshot.fileDeletionInProgress);
    }
    if (directMediaScopeChanged || !directMediaNavigationActive()) {
        refreshDirectMediaNavigation();
    } else if (m_state.directMediaNavigationKnown()) {
        cacheDisplayedMediaPredecodeImages();
    }

    if (!sameActiveNavigationSnapshot(
            previousPageNavigation, m_imagePublicSnapshot.pageNavigation)) {
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        publishActiveNavigationForImagePages();
        return;
    }

    recomputePublicProjection();
}

void DocumentSessionRuntime::handleVideoDocumentSnapshotChanged()
{
    refreshVideoPublicSnapshot();
    syncFromVideoDocument();
}

void DocumentSessionRuntime::refreshImagePublicSnapshot()
{
    const DocumentSessionImageDocumentSnapshot leafSnapshot = m_imageDocument.snapshot();
    DocumentSessionPublicImageLeafSnapshot snapshot;
    const QUrl sourceUrl = leafSnapshot.sourceUrl;
    snapshot.sourceUrl = leafSnapshot.sourceUrl;
    snapshot.sourceMayRepresentDocument
        = !sourceUrl.isEmpty() && !isSupportedDirectImageUrl(sourceUrl);
    snapshot.pageNavigation = leafSnapshot.activeNavigationSnapshot;
    snapshot.pageNavigationRows = leafSnapshot.pageNavigationSnapshot;
    snapshot.displayedUrl = leafSnapshot.displayedUrl;
    snapshot.displayedOpenedCollectionScope = leafSnapshot.displayedOpenedCollectionScope;
    snapshot.windowTitleFileName = leafSnapshot.windowTitleFileName;
    snapshot.directMediaSize = leafSnapshot.primaryImageSize;
    snapshot.embeddedMetadata = leafSnapshot.embeddedMetadata;
    snapshot.readyForDeletion = leafSnapshot.ready;
    snapshot.readyForInformation = leafSnapshot.ready;
    snapshot.error = leafSnapshot.error;
    snapshot.fileDeletionInProgress = leafSnapshot.fileDeletionInProgress;
    snapshot.openedCollectionScopeActive = leafSnapshot.openedCollectionScopeActive;
    snapshot.ordinaryDirectMediaScopeActive = leafSnapshot.ordinaryDirectMediaScopeActive;
    snapshot.unsupportedOpenedCollectionVideo = leafSnapshot.unsupportedOpenedCollectionVideo;
    snapshot.directImageReplacementPending = !m_state.directMediaCursor().pendingUrl.isEmpty();
    snapshot.containerNavigationAvailable = leafSnapshot.containerNavigationAvailable;
    snapshot.twoPageModeEnabled = leafSnapshot.twoPageModeEnabled;
    snapshot.twoPageModeAvailable = leafSnapshot.twoPageModeAvailable;
    snapshot.rightToLeftReadingEnabled = leafSnapshot.rightToLeftReadingEnabled;
    snapshot.rightToLeftReadingAvailable = leafSnapshot.rightToLeftReadingAvailable;
    snapshot.fitModeSelected = leafSnapshot.fitModeSelected;
    snapshot.fitHeightModeSelected = leafSnapshot.fitHeightModeSelected;
    snapshot.fitWidthModeSelected = leafSnapshot.fitWidthModeSelected;
    snapshot.zoomPercentKnown = leafSnapshot.zoomPercentKnown;
    snapshot.zoomPercent = leafSnapshot.zoomPercent;
    snapshot.errorString = leafSnapshot.errorString;
    snapshot.primaryDisplayedPredecodeImage = leafSnapshot.primaryDisplayedPredecodeImage;
    snapshot.firstDisplayDecodeContext = leafSnapshot.firstDisplayDecodeContext;
    m_imagePublicSnapshot = std::move(snapshot);
}

void DocumentSessionRuntime::refreshVideoPublicSnapshot()
{
    const DocumentSessionVideoDocumentSnapshot leafSnapshot = m_videoDocument.snapshot();
    DocumentSessionPublicVideoLeafSnapshot snapshot;
    snapshot.sourceUrl = leafSnapshot.sourceUrl;
    snapshot.windowTitleFileName = leafSnapshot.windowTitleFileName;
    snapshot.directMediaSize = leafSnapshot.videoSize;
    snapshot.embeddedMetadata = leafSnapshot.embeddedMetadata;
    snapshot.ready = leafSnapshot.ready;
    snapshot.hasVideo = leafSnapshot.hasVideo;
    snapshot.sourcePresent = !snapshot.sourceUrl.isEmpty();
    snapshot.error = leafSnapshot.error;
    snapshot.zoomPercentKnown = leafSnapshot.zoomPercentKnown;
    snapshot.zoomPercent = leafSnapshot.zoomPercent;
    snapshot.errorString = leafSnapshot.errorString;
    m_videoPublicSnapshot = std::move(snapshot);
}

void DocumentSessionRuntime::refreshLeafPublicSnapshots()
{
    refreshImagePublicSnapshot();
    refreshVideoPublicSnapshot();
}

void DocumentSessionRuntime::syncImageDocumentFileDeletionProgress()
{
    if (m_state.documentKind() != DocumentSessionKind::Image
        || directImageLoadMayUseImageDocumentSourceScope()) {
        return;
    }

    m_state.setFileDeletionInProgress(m_imagePublicSnapshot.fileDeletionInProgress);
    recomputePublicProjection();
}

void DocumentSessionRuntime::setDocumentKind(DocumentSessionKind kind)
{
    m_state.setDocumentKindAndActiveZoomSnapshot(kind, activeZoomSnapshotForKind(kind));
}

void DocumentSessionRuntime::recomputeActiveZoomReadout() { recomputePublicProjection(); }

void DocumentSessionRuntime::recomputeActiveZoomReadoutForKind(DocumentSessionKind kind)
{
    if (m_routingSource) {
        return;
    }

    if (m_state.documentKind() == kind) {
        recomputePublicProjection();
    }
}

void DocumentSessionRuntime::publishActiveNavigationForImagePages()
{
    setActiveNavigationRevealContext(
        takePendingActiveNavigationRevealContext(ActiveNavigationRevealIntent::ProgrammaticSync));
    if (m_state.updatePublicSnapshotForSourceKind(
            publicSnapshotInput(++m_publicSnapshotInputRevision),
            ActiveNavigationSourceKind::ImageDocumentPages)) {
        syncActiveNavigationThumbnailRows();
    }
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionRuntime::recomputePublicProjection()
{
    m_state.updatePublicSnapshot(publicSnapshotInput(++m_publicSnapshotInputRevision));
    syncActiveNavigationThumbnailRows();
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionRuntime::syncActiveNavigationThumbnailRows()
{
    std::vector<ActiveNavigationThumbnailRow> rows = projectActiveNavigationThumbnailRows(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(),
        m_state.directMediaNavigationCandidates(), m_imagePublicSnapshot.pageNavigationRows);
    if (rows.empty()) {
        m_activeNavigationThumbnailRuntime.setRows({});
        return;
    }

    m_activeNavigationThumbnailRuntime.setRows(std::move(rows));
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    setPendingActiveNavigationRevealContext(
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::LoadOrOpen });
    const DocumentSessionRoutePlan plan
        = documentSessionRoutePlanForSourceUrl(sourceUrl, m_state.documentKind());
    qCDebug(kiriviewNavigationLog)
        << "route source url"
        << "url" << sourceUrl << "currentKind" << documentKindName(m_state.documentKind())
        << "routeKind" << routeKindName(plan.kind) << "operations" << plan.operations.size();
    executeRoutePlan(plan);
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    const DocumentSessionRoutePlan plan
        = documentSessionRoutePlanForMediaUrl(url, m_state.documentKind());
    qCDebug(kiriviewNavigationLog)
        << "route media url"
        << "url" << url << "currentKind" << documentKindName(m_state.documentKind()) << "routeKind"
        << routeKindName(plan.kind) << "operations" << plan.operations.size();
    executeRoutePlan(plan);
}

void DocumentSessionRuntime::executeRoutePlan(const DocumentSessionRoutePlan &plan)
{
    qCDebug(kiriviewNavigationLog)
        << "execute route plan"
        << "routeKind" << routeKindName(plan.kind) << "sourceUrl" << plan.sourceUrl
        << "documentKindBefore" << documentKindName(m_state.documentKind());
    m_routeRuntime.execute(plan);
    qCDebug(kiriviewNavigationLog)
        << "execute route plan complete"
        << "routeKind" << routeKindName(plan.kind) << "documentKindAfter"
        << documentKindName(m_state.documentKind()) << "sourceUrl" << m_state.sourceUrl()
        << "activeNavigationAvailable" << m_state.activeNavigationSnapshot().available
        << "activeNavigationKnown" << m_state.activeNavigationSnapshot().known
        << "activeNavigationCurrent" << m_state.activeNavigationSnapshot().currentNumber
        << "activeNavigationCount" << m_state.activeNavigationSnapshot().count;
}

void DocumentSessionRuntime::leaveVideoMode()
{
    if (m_videoPublicSnapshot.sourceUrl.isEmpty() && m_videoDocument.videoOutput() == nullptr) {
        return;
    }

    m_videoOutputRuntime.clear();
    m_videoDocument.stop();
    m_videoDocument.setVideoOutput(nullptr);
    m_videoDocument.setSourceUrl(QUrl());
}

void DocumentSessionRuntime::syncFromImageDocument()
{
    if (m_routingSource || m_state.documentKind() != DocumentSessionKind::Image) {
        return;
    }

    const bool directMediaScopeChanged = syncDirectImageCursorFromDocument();
    m_state.setSourceIdentity(m_imagePublicSnapshot.sourceUrl);
    recomputeActiveZoomReadout();
    recomputePublicProjection();
    if (directMediaScopeChanged) {
        refreshDirectMediaNavigation();
    } else if (m_state.directMediaNavigationKnown()) {
        cacheDisplayedMediaPredecodeImages();
    }
}

void DocumentSessionRuntime::syncFromVideoDocument()
{
    if (m_routingSource || m_state.documentKind() != DocumentSessionKind::Video) {
        return;
    }

    if (m_videoPublicSnapshot.sourceUrl.isEmpty()) {
        qCDebug(kiriviewNavigationLog) << "sync from video document"
                                       << "state" << "empty-source";
        m_state.clearDirectMediaCursor();
        m_state.setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        m_state.setDirectMediaNavigation({}, false, {});
    } else {
        const bool directMediaScopeChanged
            = m_state.setDirectVideoCursor(m_videoPublicSnapshot.sourceUrl);
        logDirectMediaScope("sync from video document", m_state.directMediaScope());
        m_state.setSourceIdentity(m_videoPublicSnapshot.sourceUrl);
        if (directMediaScopeChanged) {
            refreshDirectMediaNavigation();
        }
    }

    recomputePublicProjection();
    recomputeActiveZoomReadout();
}

void DocumentSessionRuntime::refreshDirectMediaNavigation()
{
    if (!directMediaNavigationActive()) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation refresh skipped"
                                       << "reason"
                                       << "inactive"
                                       << "documentKind" << documentKindName(m_state.documentKind())
                                       << "cursorUrl" << activeDirectMediaCursorUrl();
        m_state.setDirectMediaNavigation({}, false, {});
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        recomputePublicProjection();
        if (!directImageLoadMayUseImageDocumentSourceScope()) {
            m_mediaPredecodeRuntime.clear();
        }
        return;
    }

    const DirectMediaScope scope = directMediaNavigationLoadScope();
    logDirectMediaScope("direct media navigation refresh requested", scope);
    m_directMediaNavigationRuntime.refresh(
        m_owner, scope,
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        [this](DocumentSessionDirectMediaNavigationRefreshResult result) {
            updateDirectMediaNavigationBoundaryState(std::move(result));
        });
}

void DocumentSessionRuntime::finishDirectMediaNavigation(
    DocumentSessionDirectMediaNavigationOpenResult result)
{
    const QString errorString = result.errorString;
    const DocumentSessionDirectMediaNavigationOpenApplication application
        = documentSessionDirectMediaNavigationOpenApplication(
            activeDirectMediaCursorUrl(), std::move(result));
    if (!application.known) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation open failed"
                                       << "error" << errorString;
        m_state.setDirectMediaNavigation(application.boundaryState, false, application.candidates);
        applyDirectMediaNavigationRevealAction(application.revealAction);
        recomputePublicProjection();
        return;
    }

    m_state.setDirectMediaNavigation(
        application.boundaryState, application.known, application.candidates);
    qCDebug(kiriviewNavigationLog)
        << "direct media navigation open finished"
        << "candidates" << application.candidates.size() << "currentNumber"
        << application.boundaryState.currentNumber << "count" << application.boundaryState.count
        << "targetUrl" << application.routeTargetUrl.value_or(QUrl());
    applyDirectMediaNavigationRevealAction(application.revealAction);
    recomputePublicProjection();
    if (application.schedulePredecode) {
        scheduleMediaPredecode(application.candidates);
    }
    if (application.routeTargetUrl.has_value()) {
        openMediaUrl(*application.routeTargetUrl);
    }
}

void DocumentSessionRuntime::updateDirectMediaNavigationBoundaryState(
    DocumentSessionDirectMediaNavigationRefreshResult result)
{
    const QString errorString = result.errorString;
    const DocumentSessionDirectMediaNavigationRefreshApplication application
        = documentSessionDirectMediaNavigationRefreshApplication(
            m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(),
            std::move(result));
    if (!application.known) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation refresh failed"
                                       << "error" << errorString;
        m_state.setDirectMediaNavigation(application.boundaryState, false, application.candidates);
        applyDirectMediaNavigationRevealAction(application.revealAction);
        recomputePublicProjection();
        return;
    }

    m_state.setDirectMediaNavigation(
        application.boundaryState, application.known, application.candidates);
    qCDebug(kiriviewNavigationLog)
        << "direct media navigation refresh finished"
        << "candidates" << application.candidates.size() << "currentNumber"
        << application.boundaryState.currentNumber << "count" << application.boundaryState.count
        << "canPrevious" << application.boundaryState.canOpenPrevious << "canNext"
        << application.boundaryState.canOpenNext;
    applyDirectMediaNavigationRevealAction(application.revealAction);
    recomputePublicProjection();
    if (application.schedulePredecode) {
        scheduleMediaPredecode(application.candidates);
    }
}

void DocumentSessionRuntime::scheduleMediaPredecode(
    const std::vector<DirectMediaNavigationCandidate> &candidates)
{
    m_mediaPredecodeRuntime.schedule(mediaPredecodeInput(), candidates);
}

void DocumentSessionRuntime::cacheDisplayedMediaPredecodeImages()
{
    m_mediaPredecodeRuntime.cacheDisplayedImages(mediaPredecodeInput());
}

DocumentSessionMediaPredecodeInput DocumentSessionRuntime::mediaPredecodeInput() const
{
    return DocumentSessionMediaPredecodeInput {
        directMediaNavigationActive(),
        m_state.documentKind(),
        activeImageUsesImageDocumentSourceScope(),
        m_imagePublicSnapshot.readyForInformation,
        activeDirectMediaCursorUrl(),
        m_imagePublicSnapshot.primaryDisplayedPredecodeImage,
        m_imagePublicSnapshot.firstDisplayDecodeContext,
    };
}

void DocumentSessionRuntime::cancelMediaDeletion()
{
    const bool sessionMediaDeletionInProgress
        = m_state.fileDeletionInProgress() && directMediaNavigationActive();
    if (!m_mediaDeletionRuntime.active() && !sessionMediaDeletionInProgress) {
        return;
    }

    m_mediaDeletionRuntime.cancel();
    m_state.setFileDeletionInProgress(false);
    recomputePublicProjection();
}

MediaOpenWithPlan DocumentSessionRuntime::currentMediaOpenWithPlan() const
{
    return mediaOpenWithPlan(MediaOpenWithPlanInput {
        m_state.documentKind(),
        m_imagePublicSnapshot.readyForInformation,
        m_imagePublicSnapshot.displayedUrl,
        m_imagePublicSnapshot.displayedOpenedCollectionScope,
        m_videoPublicSnapshot.ready,
        m_videoPublicSnapshot.sourceUrl,
    });
}

void DocumentSessionRuntime::cancelMediaOpenWith() { m_mediaOpenWithRuntime.cancel(); }

void DocumentSessionRuntime::finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion)
{
    m_state.setFileDeletionInProgress(false);
    if (completion.plan.reportFailure) {
        m_state.setSessionErrorString(completion.failure.userMessage.isEmpty()
                ? genericFileDeletionErrorMessage()
                : completion.failure.userMessage);
        recomputePublicProjection();
        return;
    }

    recomputePublicProjection();

    executeMediaDeletionCompletionPlan(completion.plan, completion.failure.userMessage);
}

void DocumentSessionRuntime::executeMediaDeletionCompletionPlan(
    const DocumentSessionMediaDeletionCompletionPlan &plan, const QString &)
{
    if (plan.reportFailure) {
        return;
    }

    if (plan.hasRoutePlan()) {
        executeRoutePlan(plan.routePlan);
        return;
    }
}

DirectMediaScope DocumentSessionRuntime::directMediaNavigationLoadScope() const
{
    return m_state.directMediaScope();
}

QUrl DocumentSessionRuntime::activeDirectMediaCursorUrl() const
{
    return m_state.directMediaCursorUrl();
}

bool DocumentSessionRuntime::directMediaCursorMatches(const DirectMediaScope &scope) const
{
    return directMediaScopeMatchesCursor(m_state.directMediaCursor(), scope);
}

bool DocumentSessionRuntime::activeImageUsesImageDocumentSourceScope() const
{
    return m_imagePublicSnapshot.ordinaryDirectMediaScopeActive;
}

bool DocumentSessionRuntime::directImageLoadMayUseImageDocumentSourceScope() const
{
    return m_state.documentKind() == DocumentSessionKind::Image
        && !activeDirectMediaCursorUrl().isEmpty();
}

bool DocumentSessionRuntime::syncDirectImageCursorFromDocument()
{
    if (m_state.documentKind() != DocumentSessionKind::Image) {
        return false;
    }

    const QUrl pendingUrl = m_state.directMediaCursor().pendingUrl;
    const QUrl displayedUrl = m_imagePublicSnapshot.displayedUrl;
    if (!pendingUrl.isEmpty()) {
        if (activeImageUsesImageDocumentSourceScope()
            && sameNormalizedUrl(displayedUrl, pendingUrl)) {
            return m_state.confirmDirectImageCursor(displayedUrl);
        }

        if (m_imagePublicSnapshot.error) {
            if (sameNormalizedUrl(m_imagePublicSnapshot.sourceUrl, pendingUrl)) {
                return m_state.confirmDirectImageCursor(pendingUrl);
            }
            return m_state.restoreDirectImageCursorAfterFailure();
        }

        if (!m_imagePublicSnapshot.sourceUrl.isEmpty()
            && m_imagePublicSnapshot.sourceUrl != pendingUrl) {
            return m_state.restoreDirectImageCursorAfterFailure();
        }
        return false;
    }

    if (activeImageUsesImageDocumentSourceScope() && !displayedUrl.isEmpty()) {
        return m_state.confirmDirectImageCursor(displayedUrl);
    }

    if (m_imagePublicSnapshot.error) {
        return m_state.restoreDirectImageCursorAfterFailure();
    }

    return false;
}

ActiveZoomSnapshot DocumentSessionRuntime::activeZoomSnapshotForKind(DocumentSessionKind kind) const
{
    switch (kind) {
    case DocumentSessionKind::Image:
        if (!m_imagePublicSnapshot.zoomPercentKnown) {
            return {};
        }
        return ActiveZoomSnapshot { true, true, m_imagePublicSnapshot.zoomPercent, true };
    case DocumentSessionKind::Video:
        return ActiveZoomSnapshot {
            true,
            m_videoPublicSnapshot.zoomPercentKnown,
            m_videoPublicSnapshot.zoomPercentKnown ? qreal(m_videoPublicSnapshot.zoomPercent) : 0.0,
            false,
        };
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

DocumentSessionPublicSnapshotInput DocumentSessionRuntime::publicSnapshotInput(
    quint64 inputRevision) const
{
    DocumentSessionPublicSnapshotInput input;
    input.inputRevision = inputRevision;
    input.session.sourceUrl = m_state.sourceUrl();
    input.session.documentKind = m_state.documentKind();
    input.session.sessionErrorString = m_state.sessionErrorString();
    input.session.fileDeletionInProgress = m_state.fileDeletionInProgress();
    input.session.directImageLoadMayUseImageDocumentSourceScope
        = directImageLoadMayUseImageDocumentSourceScope();
    input.session.directMediaNavigation = directMediaActiveNavigationInput();
    input.session.activeNavigationRevealIntent = m_state.activeNavigationRevealIntent();
    input.session.activeNavigationRevealDirection = m_state.activeNavigationRevealDirection();

    input.image = m_imagePublicSnapshot;
    input.image.directImageReplacementPending = !m_state.directMediaCursor().pendingUrl.isEmpty();
    input.video = m_videoPublicSnapshot;
    input.operations = operationAvailabilitySnapshot();
    return input;
}

DirectMediaActiveNavigationInput DocumentSessionRuntime::directMediaActiveNavigationInput() const
{
    return DirectMediaActiveNavigationInput { m_state.directMediaNavigationState(),
        m_state.directMediaNavigationKnown() };
}

DocumentSessionPublicOperationAvailabilitySnapshot
DocumentSessionRuntime::operationAvailabilitySnapshot() const
{
    return DocumentSessionPublicOperationAvailabilitySnapshot {
        currentMediaOpenWithPlan().hasRequest(),
    };
}
}
