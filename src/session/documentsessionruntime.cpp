// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "navigation/navigationlogging.h"
#include "session/documentsessionactivezoom.h"

#include <QAbstractListModel>
#include <QDebug>
#include <QObject>
#include <QScopedValueRollback>
#include <QString>
#include <utility>

namespace {
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

}

namespace kiriview {
DocumentSessionRuntime::DocumentSessionRuntime(QObject *owner,
    DocumentSessionImageDocumentSnapshotPort imageDocument,
    DocumentSessionImageDocumentCommandPort imageCommands,
    DocumentSessionVideoDocumentSnapshotPort videoDocument,
    DocumentSessionVideoDocumentCommandPort videoCommands, ChangeCallback changeCallback,
    DocumentSessionRuntimeDependencies dependencies)
    : m_owner(owner)
    , m_imageDocument(std::move(imageDocument))
    , m_imageDocumentCommandRuntime(std::move(imageCommands))
    , m_videoDocument(std::move(videoDocument))
    , m_videoDocumentCommandRuntime(std::move(videoCommands),
          [this](const DocumentSessionVideoOutputAttachmentPort &attachmentPort) {
              m_videoOutputRuntime.clearAttachment(attachmentPort);
          })
    , m_state(std::move(changeCallback))
    , m_projectionRuntime(DocumentSessionProjectionRuntimePorts {
          [this](const DocumentSessionPublicSnapshotInput &input) {
              return m_state.updatePublicSnapshot(input);
          },
          [this](const DocumentSessionPublicSnapshotInput &input,
              ActiveNavigationSourceKind sourceKind) {
              return m_state.updatePublicSnapshotForSourceKind(input, sourceKind);
          },
          [this]() { return m_state.activeNavigationSourceKind(); },
          [this]() { return m_state.activeNavigationSnapshot(); },
          [this]() { return m_state.directMediaNavigationCandidates(); },
          [this](std::vector<ActiveNavigationThumbnailRow> rows) {
              m_activeNavigationThumbnailRuntime.setRows(std::move(rows));
          },
          [this]() { clearActiveNavigationRevealContextIfUnavailable(); },
      })
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
              m_imageDocumentCommandRuntime.clearSourceUrl();
              refreshImagePublicSnapshot();
          },
          [this]() {
              leaveVideoMode();
              refreshVideoPublicSnapshot();
          },
          [this]() { setDocumentKind(DocumentSessionKind::Empty); },
          [this](const QUrl &url) {
              m_imageDocumentCommandRuntime.setSourceUrl(url);
              refreshImagePublicSnapshot();
              setDocumentKind(DocumentSessionKind::Image);
          },
          [this](const QUrl &url) {
              m_videoDocumentCommandRuntime.setSourceUrl(url);
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
          [this]() { m_imageDocumentCommandRuntime.openPreviousPage(); },
          [this]() { m_imageDocumentCommandRuntime.openNextPage(); },
          [this](int number) { m_imageDocumentCommandRuntime.openImageAtPage(number); },
      })
    , m_activeNavigationThumbnailRuntime(
          owner, &m_imageDocument, std::move(dependencies.activeNavigationThumbnails))
    , m_directMediaNavigationRuntime(dependencies.directMediaNavigationCandidateProvider)
    , m_directMediaNavigationApplicationRuntime(
          DocumentSessionDirectMediaNavigationApplicationPorts {
              [this](DirectMediaNavigationBoundaryState state, bool known,
                  std::vector<DirectMediaNavigationCandidate> candidates) {
                  m_state.setDirectMediaNavigation(state, known, std::move(candidates));
              },
              [this](DocumentSessionDirectMediaNavigationRevealAction action) {
                  applyDirectMediaNavigationRevealAction(action);
              },
              [this]() { recomputePublicProjection(); },
              [this]() { m_mediaPredecodeRuntime.clear(); },
              [this](const std::vector<DirectMediaNavigationCandidate> &candidates) {
                  scheduleMediaPredecode(candidates);
              },
              [this](const QUrl &url) { openMediaUrl(url); },
          })
    , m_mediaDeletionRuntime(std::move(dependencies.fileDeletionProvider),
          std::move(dependencies.directMediaNavigationCandidateProvider))
    , m_mediaDeletionCompletionRuntime(DocumentSessionMediaDeletionCompletionRuntimePorts {
          [this](bool inProgress) { m_state.setFileDeletionInProgress(inProgress); },
          [this](const QString &errorString) { m_state.setSessionErrorString(errorString); },
          [this]() { recomputePublicProjection(); },
          [this](const DocumentSessionRoutePlan &plan) { executeRoutePlan(plan); },
      })
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

    return m_videoOutputRuntime.reportSurfaceClaim(
        { claimToken, surfaceOwner, videoOutput, attach, contentRect, sourceRect },
        videoOutputAttachmentPort());
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
        m_imageDocumentCommandRuntime.deleteDisplayedFile(mode);
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
    const DocumentSessionImageDocumentSyncPlan plan
        = documentSessionImageDocumentSyncPlan(DocumentSessionImageDocumentSyncInput {
            m_routingSource,
            m_state.documentKind(),
            directImageLoadMayUseImageDocumentSourceScope(),
            directMediaNavigationActive(),
            m_state.directMediaNavigationKnown(),
            directMediaScopeChanged,
            previousPageNavigation,
            m_imagePublicSnapshot,
        });
    if (!plan.active) {
        return;
    }

    if (plan.setSourceIdentity) {
        m_state.setSourceIdentity(plan.sourceIdentityUrl);
    }
    if (plan.syncFileDeletionProgress) {
        m_state.setFileDeletionInProgress(plan.fileDeletionInProgress);
    }

    switch (plan.directMediaOperation) {
    case DocumentSessionImageDocumentSyncDirectMediaOperation::None:
        break;
    case DocumentSessionImageDocumentSyncDirectMediaOperation::RefreshDirectMediaNavigation:
        refreshDirectMediaNavigation();
        break;
    case DocumentSessionImageDocumentSyncDirectMediaOperation::CacheDisplayedMediaPredecodeImages:
        cacheDisplayedMediaPredecodeImages();
        break;
    }

    switch (plan.projectionOperation) {
    case DocumentSessionImageDocumentSyncProjectionOperation::None:
        return;
    case DocumentSessionImageDocumentSyncProjectionOperation::RecomputePublicProjection:
        recomputePublicProjection();
        return;
    case DocumentSessionImageDocumentSyncProjectionOperation::PublishImagePageActiveNavigation:
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        publishActiveNavigationForImagePages();
        return;
    }
}

void DocumentSessionRuntime::handleVideoDocumentSnapshotChanged()
{
    refreshVideoPublicSnapshot();
    syncFromVideoDocument();
}

void DocumentSessionRuntime::refreshImagePublicSnapshot()
{
    m_imagePublicSnapshot = buildDocumentSessionPublicImageLeafSnapshot(m_imageDocument.snapshot());
}

void DocumentSessionRuntime::refreshVideoPublicSnapshot()
{
    m_videoPublicSnapshot = buildDocumentSessionPublicVideoLeafSnapshot(m_videoDocument.snapshot());
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
    m_projectionRuntime.publishForSourceKind(publicSnapshotInput(++m_publicSnapshotInputRevision),
        ActiveNavigationSourceKind::ImageDocumentPages, m_imagePublicSnapshot.pageNavigationRows);
}

void DocumentSessionRuntime::recomputePublicProjection()
{
    m_projectionRuntime.publish(publicSnapshotInput(++m_publicSnapshotInputRevision),
        m_imagePublicSnapshot.pageNavigationRows);
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    setPendingActiveNavigationRevealContext(
        ActiveNavigationRevealContext { ActiveNavigationRevealIntent::LoadOrOpen });
    m_routeRuntime.routeSourceUrl(sourceUrl, m_state.documentKind());
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    m_routeRuntime.routeMediaUrl(url, m_state.documentKind());
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
    m_videoDocumentCommandRuntime.leaveMode(m_videoPublicSnapshot.sourceUrl);
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

    const DocumentSessionVideoDocumentSyncPlan plan = documentSessionVideoDocumentSyncPlan(
        DocumentSessionVideoDocumentSyncInput { m_state.documentKind(), m_videoPublicSnapshot });
    switch (plan.operation) {
    case DocumentSessionVideoDocumentSyncOperation::None:
        break;
    case DocumentSessionVideoDocumentSyncOperation::ClearSessionDirectMedia:
        qCDebug(kiriviewNavigationLog) << "sync from video document"
                                       << "state" << "empty-source";
        m_state.clearDirectMediaCursor();
        m_state.setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        m_state.setDirectMediaNavigation({}, false, {});
        break;
    case DocumentSessionVideoDocumentSyncOperation::CommitDirectVideoCursor: {
        const bool directMediaScopeChanged = m_state.setDirectVideoCursor(plan.url);
        logDirectMediaScope("sync from video document", m_state.directMediaScope());
        m_state.setSourceIdentity(plan.url);
        if (directMediaScopeChanged) {
            refreshDirectMediaNavigation();
        }
        break;
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
        m_directMediaNavigationApplicationRuntime.applyInactiveRefresh(
            !directImageLoadMayUseImageDocumentSourceScope());
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
    m_directMediaNavigationApplicationRuntime.applyOpen(
        activeDirectMediaCursorUrl(), std::move(result));
}

void DocumentSessionRuntime::updateDirectMediaNavigationBoundaryState(
    DocumentSessionDirectMediaNavigationRefreshResult result)
{
    m_directMediaNavigationApplicationRuntime.applyRefresh(m_state.activeNavigationSourceKind(),
        m_state.activeNavigationSnapshot(), std::move(result));
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

DocumentSessionVideoOutputAttachmentPort DocumentSessionRuntime::videoOutputAttachmentPort()
{
    return m_videoDocumentCommandRuntime.outputAttachmentPort();
}

void DocumentSessionRuntime::finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion)
{
    m_mediaDeletionCompletionRuntime.apply(completion);
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
    const DocumentSessionDirectImageCursorSyncPlan plan
        = documentSessionDirectImageCursorSyncPlan(DocumentSessionDirectImageCursorSyncInput {
            m_state.documentKind(),
            m_state.directMediaCursor(),
            m_imagePublicSnapshot,
        });
    switch (plan.operation) {
    case DocumentSessionDirectImageCursorSyncOperation::None:
        return false;
    case DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor:
        return m_state.confirmDirectImageCursor(plan.url);
    case DocumentSessionDirectImageCursorSyncOperation::RestoreDirectImageCursorAfterFailure:
        return m_state.restoreDirectImageCursorAfterFailure();
    }

    return false;
}

ActiveZoomSnapshot DocumentSessionRuntime::activeZoomSnapshotForKind(DocumentSessionKind kind) const
{
    return documentSessionActiveZoomSnapshot(kind, m_imagePublicSnapshot, m_videoPublicSnapshot);
}

DocumentSessionPublicSnapshotInput DocumentSessionRuntime::publicSnapshotInput(
    quint64 inputRevision) const
{
    DocumentSessionPublicSnapshotInputBuilderInput input;
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
    input.video = m_videoPublicSnapshot;
    input.directMediaCursor = m_state.directMediaCursor();
    return buildDocumentSessionPublicSnapshotInput(input);
}

DirectMediaActiveNavigationInput DocumentSessionRuntime::directMediaActiveNavigationInput() const
{
    return DirectMediaActiveNavigationInput { m_state.directMediaNavigationState(),
        m_state.directMediaNavigationKnown() };
}

}
