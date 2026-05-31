// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "localization/imageerrortext.h"
#include "location/imageurl.h"
#include "navigation/mediaformatregistry.h"
#include "navigation/navigationlogging.h"
#include "predecode/mediapredecodecoordinator.h"
#include "session/activenavigationthumbnailprojection.h"

#include <QAbstractListModel>
#include <QDebug>
#include <QObject>
#include <QScopedValueRollback>
#include <QString>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
QString genericFileDeletionErrorMessage()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DeleteFile);
}

const char *documentKindName(KiriView::DocumentSessionKind kind)
{
    switch (kind) {
    case KiriView::DocumentSessionKind::Empty:
        return "Empty";
    case KiriView::DocumentSessionKind::Image:
        return "Image";
    case KiriView::DocumentSessionKind::Video:
        return "Video";
    }

    return "Unknown";
}

const char *routeKindName(KiriView::DocumentSessionRouteKind kind)
{
    switch (kind) {
    case KiriView::DocumentSessionRouteKind::Empty:
        return "Empty";
    case KiriView::DocumentSessionRouteKind::DirectVideo:
        return "DirectVideo";
    case KiriView::DocumentSessionRouteKind::DirectImage:
        return "DirectImage";
    case KiriView::DocumentSessionRouteKind::ImageDocument:
        return "ImageDocument";
    }

    return "Unknown";
}

void logDirectMediaScope(const char *message, const KiriView::DirectMediaScope &scope)
{
    qCDebug(kiriviewNavigationLog) << message << "currentUrl" << scope.currentUrl << "parentUrl"
                                   << scope.parentUrl << "generation" << scope.generation;
}

void appendConnection(std::vector<QMetaObject::Connection> &connections,
    const KiriView::DocumentSessionDocumentSignalConnector &connector, QObject *owner,
    KiriView::DocumentSessionDocumentChangeHandler handler)
{
    if (connector) {
        connections.push_back(connector(owner, std::move(handler)));
    }
}
}

namespace KiriView {
DocumentSessionRuntime::DocumentSessionRuntime(QObject *owner,
    DocumentSessionImageDocumentPort imageDocument, DocumentSessionVideoDocumentPort videoDocument,
    ChangeCallback changeCallback, DocumentSessionRuntimeDependencies dependencies)
    : m_owner(owner)
    , m_imageDocument(std::move(imageDocument))
    , m_videoDocument(std::move(videoDocument))
    , m_state(std::move(changeCallback))
    , m_activeNavigationThumbnailModel(std::make_unique<ActiveNavigationThumbnailModel>(owner))
    , m_directMediaNavigationRuntime(dependencies.directMediaNavigationCandidateProvider)
    , m_directMediaDeletionCandidateRuntime(
          std::move(dependencies.directMediaNavigationCandidateProvider))
    , m_mediaDeletionRuntime(std::move(dependencies.fileDeletionProvider))
    , m_mediaOpenWithProvider(
          mediaOpenWithProviderWithDefault(std::move(dependencies.mediaOpenWithProvider)))
    , m_mediaPredecodeCoordinator(std::make_unique<MediaPredecodeCoordinator>(owner,
          resolveMediaPredecodeDependencies(
              std::move(dependencies.directMediaPredecodeDependencies))))
{
    connectDocuments();
}

DocumentSessionRuntime::~DocumentSessionRuntime()
{
    for (const QMetaObject::Connection &connection : m_documentConnections) {
        QObject::disconnect(connection);
    }
    m_directMediaNavigationRuntime.cancel();
    m_directMediaDeletionCandidateRuntime.cancel();
    m_mediaDeletionRuntime.cancel();
    m_mediaOpenWithJob.cancel();
}

QUrl DocumentSessionRuntime::sourceUrl() const { return m_state.sourceUrl(); }

void DocumentSessionRuntime::setSourceUrl(const QUrl &sourceUrl) { routeSourceUrl(sourceUrl); }

DocumentSessionKind DocumentSessionRuntime::documentKind() const { return m_state.documentKind(); }

QString DocumentSessionRuntime::errorString() const
{
    if (!m_state.sessionErrorString().isEmpty()) {
        return m_state.sessionErrorString();
    }

    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return m_imageDocument.errorString();
    case DocumentSessionKind::Video:
        return m_videoDocument.errorString();
    case DocumentSessionKind::Empty:
        return QString();
    }

    return QString();
}

QString DocumentSessionRuntime::windowTitleSubject() const { return m_state.windowTitleSubject(); }

bool DocumentSessionRuntime::displayedFileDeletionAvailable() const
{
    return m_state.displayedFileDeletionAvailable();
}

bool DocumentSessionRuntime::displayedMediaOpenWithAvailable() const
{
    return m_state.displayedMediaOpenWithAvailable();
}

bool DocumentSessionRuntime::fileDeletionInProgress() const
{
    return m_state.fileDeletionInProgress();
}

bool DocumentSessionRuntime::activeZoomPercentAvailable() const
{
    return m_state.activeZoomSnapshot().available;
}

bool DocumentSessionRuntime::activeZoomPercentKnown() const
{
    return m_state.activeZoomSnapshot().known;
}

qreal DocumentSessionRuntime::activeZoomPercent() const
{
    return m_state.activeZoomSnapshot().percent;
}

bool DocumentSessionRuntime::activeZoomEditable() const
{
    return m_state.activeZoomSnapshot().editable;
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

ActiveNavigationBoundaryScope DocumentSessionRuntime::activeNavigationBoundaryScope() const
{
    return m_state.activeNavigationBoundaryScope();
}

ActiveNavigationRevealIntent DocumentSessionRuntime::activeNavigationRevealIntent() const
{
    return m_state.activeNavigationRevealIntent();
}

ActiveNavigationRevealDirection DocumentSessionRuntime::activeNavigationRevealDirection() const
{
    return m_state.activeNavigationRevealDirection();
}

QAbstractListModel *DocumentSessionRuntime::activeNavigationThumbnailModel() const
{
    return m_activeNavigationThumbnailModel.get();
}

std::optional<PredecodedImage> DocumentSessionRuntime::findPredecodedImage(const QUrl &url) const
{
    return m_mediaPredecodeCoordinator != nullptr
        ? m_mediaPredecodeCoordinator->findPredecodedImage(url)
        : std::optional<PredecodedImage>();
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
    const ActiveNavigationDispatchPlan plan = activeNavigationDispatchPlan(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(), request);
    if (plan.shouldDispatch()) {
        setPendingActiveNavigationRevealContext(context);
    } else {
        m_pendingActiveNavigationRevealContext = {};
        setActiveNavigationRevealContext({});
    }
    executeActiveNavigationDispatchPlan(plan);
    return plan.outcome;
}

void DocumentSessionRuntime::setPendingActiveNavigationRevealContext(
    ActiveNavigationRevealContext context)
{
    m_pendingActiveNavigationRevealContext = context;
    setActiveNavigationRevealContext(context);
}

ActiveNavigationRevealContext DocumentSessionRuntime::takePendingActiveNavigationRevealContext(
    ActiveNavigationRevealIntent fallbackIntent)
{
    const ActiveNavigationRevealContext context
        = m_pendingActiveNavigationRevealContext.intent == ActiveNavigationRevealIntent::None
        ? ActiveNavigationRevealContext { fallbackIntent }
        : m_pendingActiveNavigationRevealContext;
    m_pendingActiveNavigationRevealContext = {};
    return context;
}

void DocumentSessionRuntime::setActiveNavigationRevealContext(ActiveNavigationRevealContext context)
{
    m_state.setActiveNavigationRevealIntent(context.intent);
    m_state.setActiveNavigationRevealDirection(context.direction);
}

void DocumentSessionRuntime::clearActiveNavigationRevealContextIfUnavailable()
{
    const ActiveNavigationSnapshot &snapshot = m_state.activeNavigationSnapshot();
    if (!snapshot.available || !snapshot.known) {
        setActiveNavigationRevealContext({});
    }
}

void DocumentSessionRuntime::executeActiveNavigationDispatchPlan(
    const ActiveNavigationDispatchPlan &plan)
{
    if (!plan.shouldDispatch()) {
        return;
    }

    std::visit(
        [this](const auto &operation) {
            using Operation = std::decay_t<decltype(operation)>;
            if constexpr (std::is_same_v<Operation, OpenPreviousDirectMediaNavigationOperation>) {
                openPreviousMedia();
            } else if constexpr (std::is_same_v<Operation,
                                     OpenNextDirectMediaNavigationOperation>) {
                openNextMedia();
            } else if constexpr (std::is_same_v<Operation,
                                     OpenDirectMediaNavigationAtNumberOperation>) {
                openMediaAtNumber(operation.number);
            } else if constexpr (std::is_same_v<Operation,
                                     OpenPreviousImageDocumentPageOperation>) {
                m_imageDocument.openPreviousPage();
            } else if constexpr (std::is_same_v<Operation, OpenNextImageDocumentPageOperation>) {
                m_imageDocument.openNextPage();
            } else if constexpr (std::is_same_v<Operation,
                                     OpenImageDocumentPageAtNumberOperation>) {
                m_imageDocument.openImageAtPage(operation.number);
            }
        },
        plan.operation);
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
    loadDirectMediaNavigationCandidates(
        [this, mode](DocumentSessionDirectMediaNavigationCandidatesResult result) mutable {
            startMediaDeletion(mode, std::move(result.candidates));
        });
}

void DocumentSessionRuntime::openCurrentMediaWith(MediaOpenWithCallback callback)
{
    const MediaOpenWithPlan plan = currentMediaOpenWithPlan();
    if (!plan.hasRequest()) {
        if (callback) {
            callback(MediaOpenWithResult::Failed, QString());
        }
        return;
    }

    m_mediaOpenWithJob = m_mediaOpenWithProvider(m_owner, *plan.request, std::move(callback));
}

void DocumentSessionRuntime::connectDocuments()
{
    appendConnection(m_documentConnections, m_imageDocument.notifications.sourceUrlChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.statusChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    appendConnection(m_documentConnections,
        m_imageDocument.notifications.windowTitleFileNameChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.imageSizeChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.errorStringChanged,
        m_owner, [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    appendConnection(m_documentConnections,
        m_imageDocument.notifications.imageDocumentSourceScopeChanged, m_owner, [this]() {
            if (m_routingSource) {
                return;
            }

            const bool directMediaScopeChanged = syncDirectImageCursorFromDocument();
            if (directMediaScopeChanged || !directMediaNavigationActive()) {
                refreshDirectMediaNavigation();
            }
            setActiveNavigationRevealContext(
                ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
            recomputePublicProjection();
        });
    appendConnection(m_documentConnections,
        m_imageDocument.notifications.fileDeletionInProgressChanged, m_owner,
        [this]() { syncImageDocumentFileDeletionProgress(); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.zoomPercentKnownChanged,
        m_owner, [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.zoomPercentChanged,
        m_owner, [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    appendConnection(m_documentConnections, m_imageDocument.notifications.pageNavigationChanged,
        m_owner, [this]() { publishActiveNavigationForImagePages(); });

    appendConnection(m_documentConnections, m_videoDocument.notifications.sourceUrlChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    appendConnection(m_documentConnections, m_videoDocument.notifications.statusChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    appendConnection(m_documentConnections,
        m_videoDocument.notifications.windowTitleFileNameChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    appendConnection(m_documentConnections, m_videoDocument.notifications.videoSizeChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    appendConnection(m_documentConnections, m_videoDocument.notifications.errorStringChanged,
        m_owner, [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    appendConnection(m_documentConnections, m_videoDocument.notifications.zoomPercentKnownChanged,
        m_owner, [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Video); });
    appendConnection(m_documentConnections, m_videoDocument.notifications.zoomPercentChanged,
        m_owner, [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Video); });
}

void DocumentSessionRuntime::syncImageDocumentFileDeletionProgress()
{
    if (m_state.documentKind() != DocumentSessionKind::Image
        || directImageLoadMayUseImageDocumentSourceScope()) {
        return;
    }

    m_state.setFileDeletionInProgress(m_imageDocument.fileDeletionInProgress());
    recomputePublicProjection();
}

void DocumentSessionRuntime::setDocumentKind(DocumentSessionKind kind)
{
    m_state.setDocumentKindAndActiveZoomSnapshot(kind, activeZoomSnapshotForKind(kind));
}

void DocumentSessionRuntime::recomputeActiveZoomReadout()
{
    m_state.setActiveZoomSnapshot(activeZoomSnapshotForKind(m_state.documentKind()));
}

void DocumentSessionRuntime::recomputeActiveZoomReadoutForKind(DocumentSessionKind kind)
{
    if (m_routingSource) {
        return;
    }

    if (m_state.documentKind() == kind) {
        recomputeActiveZoomReadout();
    }
}

void DocumentSessionRuntime::publishActiveNavigationForImagePages()
{
    setActiveNavigationRevealContext(
        takePendingActiveNavigationRevealContext(ActiveNavigationRevealIntent::ProgrammaticSync));
    if (m_state.updatePublicProjectionForSourceKind(
            publicProjectionInput(), ActiveNavigationSourceKind::ImageDocumentPages)) {
        syncActiveNavigationThumbnailRows();
    }
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionRuntime::recomputePublicProjection()
{
    m_state.updatePublicProjection(publicProjectionInput());
    syncActiveNavigationThumbnailRows();
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionRuntime::syncActiveNavigationThumbnailRows()
{
    if (m_activeNavigationThumbnailModel == nullptr) {
        return;
    }

    std::vector<ActiveNavigationThumbnailRow> rows = projectActiveNavigationThumbnailRows(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(),
        m_state.directMediaNavigationCandidates(), m_imageDocument.pageNavigationSnapshot());
    if (rows.empty()) {
        m_activeNavigationThumbnailModel->clear();
        return;
    }

    m_activeNavigationThumbnailModel->setRows(std::move(rows));
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
    struct RouteExecutionState {
        bool directMediaScopeChanged = false;
        bool directMediaNavigationCleared = false;
    };

    RouteExecutionState state;
    qCDebug(kiriviewNavigationLog)
        << "execute route plan"
        << "routeKind" << routeKindName(plan.kind) << "sourceUrl" << plan.sourceUrl
        << "documentKindBefore" << documentKindName(m_state.documentKind());
    const auto executeWithRoutingSuppressed = [this](auto &&mutation) {
        QScopedValueRollback<bool> routingSource(m_routingSource, true);
        mutation();
    };

    for (const DocumentSessionRouteOperation &operation : plan.operations) {
        std::visit(
            [this, &state, &executeWithRoutingSuppressed](const auto &payload) {
                using Operation = std::decay_t<decltype(payload)>;

                if constexpr (std::is_same_v<Operation, ClearSessionErrorStringRouteOperation>) {
                    m_state.setSessionErrorString(QString());
                } else if constexpr (std::is_same_v<Operation,
                                         CancelDirectMediaNavigationRouteOperation>) {
                    m_directMediaNavigationRuntime.cancel();
                } else if constexpr (std::is_same_v<Operation, CancelMediaDeletionRouteOperation>) {
                    cancelMediaDeletion();
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaNavigationRouteOperation>) {
                    m_state.setDirectMediaNavigation({}, false, {});
                    state.directMediaNavigationCleared = true;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = m_state.clearDirectMediaCursor() || state.directMediaScopeChanged;
                    logDirectMediaScope("direct media cursor cleared", m_state.directMediaScope());
                } else if constexpr (std::is_same_v<Operation,
                                         SetDirectVideoCursorRouteOperation>) {
                    state.directMediaScopeChanged = m_state.setDirectVideoCursor(payload.url)
                        || state.directMediaScopeChanged;
                    logDirectMediaScope("direct video cursor set", m_state.directMediaScope());
                } else if constexpr (std::is_same_v<Operation,
                                         RequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged = m_state.requestDirectImageCursor(payload.url)
                        || state.directMediaScopeChanged;
                    logDirectMediaScope(
                        "direct image cursor requested", m_state.directMediaScope());
                } else if constexpr (std::is_same_v<Operation,
                                         ClearThenRequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = m_state.clearDirectMediaCursor() || state.directMediaScopeChanged;
                    state.directMediaScopeChanged = m_state.requestDirectImageCursor(payload.url)
                        || state.directMediaScopeChanged;
                    logDirectMediaScope(
                        "direct image cursor cleared and requested", m_state.directMediaScope());
                } else if constexpr (std::is_same_v<Operation, ClearImageDocumentRouteOperation>) {
                    executeWithRoutingSuppressed(
                        [this]() { m_imageDocument.setSourceUrl(QUrl()); });
                } else if constexpr (std::is_same_v<Operation, LeaveVideoModeRouteOperation>) {
                    executeWithRoutingSuppressed([this]() { leaveVideoMode(); });
                } else if constexpr (std::is_same_v<Operation, EnterEmptyDocumentRouteOperation>) {
                    executeWithRoutingSuppressed(
                        [this]() { setDocumentKind(DocumentSessionKind::Empty); });
                } else if constexpr (std::is_same_v<Operation, EnterImageDocumentRouteOperation>) {
                    executeWithRoutingSuppressed([this, &payload]() {
                        m_imageDocument.setSourceUrl(payload.url);
                        setDocumentKind(DocumentSessionKind::Image);
                    });
                } else if constexpr (std::is_same_v<Operation, EnterVideoDocumentRouteOperation>) {
                    executeWithRoutingSuppressed([this, &payload]() {
                        m_videoDocument.setSourceUrl(payload.url);
                        setDocumentKind(DocumentSessionKind::Video);
                    });
                } else if constexpr (std::is_same_v<Operation,
                                         SyncDirectImageCursorFromDocumentRouteOperation>) {
                    state.directMediaScopeChanged
                        = syncDirectImageCursorFromDocument() || state.directMediaScopeChanged;
                    logDirectMediaScope(
                        "direct image cursor synced from document", m_state.directMediaScope());
                } else if constexpr (std::is_same_v<Operation, ClearSourceIdentityRouteOperation>) {
                    m_state.setSourceIdentity(QUrl());
                } else if constexpr (std::is_same_v<Operation,
                                         UseOriginalSourceIdentityRouteOperation>) {
                    m_state.setSourceIdentity(payload.url);
                } else if constexpr (std::is_same_v<Operation,
                                         UseImageDocumentSourceIdentityRouteOperation>) {
                    m_state.setSourceIdentity(m_imageDocument.sourceUrl());
                } else if constexpr (std::is_same_v<Operation,
                                         RecomputePublicProjectionRouteOperation>) {
                    recomputePublicProjection();
                } else if constexpr (std::is_same_v<Operation,
                                         RefreshDirectMediaNavigationAfterRoutingRouteOperation>) {
                    if (state.directMediaScopeChanged || state.directMediaNavigationCleared
                        || directMediaNavigationActive()) {
                        refreshDirectMediaNavigation();
                    }
                } else if constexpr (std::is_same_v<Operation, ClearMediaPredecodeRouteOperation>) {
                    if (state.directMediaNavigationCleared) {
                        m_state.setDirectMediaNavigation({}, false, {});
                        recomputePublicProjection();
                    }
                    if (m_mediaPredecodeCoordinator != nullptr) {
                        m_mediaPredecodeCoordinator->clear();
                    }
                }
            },
            operation);
    }
    qCDebug(kiriviewNavigationLog)
        << "execute route plan complete"
        << "routeKind" << routeKindName(plan.kind) << "documentKindAfter"
        << documentKindName(m_state.documentKind()) << "sourceUrl" << m_state.sourceUrl()
        << "activeNavigationAvailable" << m_state.activeNavigationSnapshot().available
        << "activeNavigationKnown" << m_state.activeNavigationSnapshot().known
        << "activeNavigationCurrent" << m_state.activeNavigationSnapshot().currentNumber
        << "activeNavigationCount" << m_state.activeNavigationSnapshot().count;
    clearActiveNavigationRevealContextIfUnavailable();
}

void DocumentSessionRuntime::leaveVideoMode()
{
    if (m_videoDocument.sourceUrl().isEmpty() && m_videoDocument.videoOutput() == nullptr) {
        return;
    }

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
    m_state.setSourceIdentity(m_imageDocument.sourceUrl());
    recomputeActiveZoomReadout();
    recomputePublicProjection();
    m_state.publish(DocumentSessionChange::ErrorString);
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

    if (m_videoDocument.sourceUrl().isEmpty()) {
        qCDebug(kiriviewNavigationLog) << "sync from video document"
                                       << "state" << "empty-source";
        m_state.clearDirectMediaCursor();
        m_state.setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        m_state.setDirectMediaNavigation({}, false, {});
    } else {
        const bool directMediaScopeChanged
            = m_state.setDirectVideoCursor(m_videoDocument.sourceUrl());
        logDirectMediaScope("sync from video document", m_state.directMediaScope());
        m_state.setSourceIdentity(m_videoDocument.sourceUrl());
        if (directMediaScopeChanged) {
            refreshDirectMediaNavigation();
        }
    }

    recomputePublicProjection();
    recomputeActiveZoomReadout();
    m_state.publish(DocumentSessionChange::ErrorString);
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
            m_mediaPredecodeCoordinator->clear();
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

void DocumentSessionRuntime::loadDirectMediaNavigationCandidates(
    DocumentSessionDirectMediaNavigationRuntime::CandidatesCallback callback)
{
    m_directMediaDeletionCandidateRuntime.loadCandidates(
        m_owner, directMediaNavigationLoadScope(),
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        std::move(callback));
}

void DocumentSessionRuntime::finishDirectMediaNavigation(
    DocumentSessionDirectMediaNavigationOpenResult result)
{
    if (!result.succeeded) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation open failed"
                                       << "error" << result.errorString;
        m_state.setDirectMediaNavigation({}, false, {});
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        recomputePublicProjection();
        return;
    }

    m_state.setDirectMediaNavigation(result.plan.boundaryState, true, result.candidates);
    qCDebug(kiriviewNavigationLog)
        << "direct media navigation open finished"
        << "candidates" << result.candidates.size() << "currentNumber"
        << result.plan.boundaryState.currentNumber << "count" << result.plan.boundaryState.count
        << "targetUrl" << result.plan.targetUrl.value_or(QUrl());
    const bool targetChangesMedia = result.plan.targetUrl.has_value()
        && !sameNormalizedUrl(*result.plan.targetUrl, activeDirectMediaCursorUrl());
    if (targetChangesMedia) {
        const ActiveNavigationRevealContext context = takePendingActiveNavigationRevealContext(
            ActiveNavigationRevealIntent::ProgrammaticSync);
        setActiveNavigationRevealContext(context);
        m_pendingActiveNavigationRevealContext = context;
    } else {
        m_pendingActiveNavigationRevealContext = {};
        setActiveNavigationRevealContext({});
    }
    recomputePublicProjection();
    scheduleMediaPredecode(result.candidates);
    if (result.plan.targetUrl.has_value()) {
        openMediaUrl(*result.plan.targetUrl);
    }
}

void DocumentSessionRuntime::updateDirectMediaNavigationBoundaryState(
    DocumentSessionDirectMediaNavigationRefreshResult result)
{
    if (!result.succeeded) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation refresh failed"
                                       << "error" << result.errorString;
        m_state.setDirectMediaNavigation({}, false, {});
        setActiveNavigationRevealContext(
            ActiveNavigationRevealContext { ActiveNavigationRevealIntent::ProgrammaticSync });
        recomputePublicProjection();
        return;
    }

    const ActiveNavigationSnapshot previousSnapshot = m_state.activeNavigationSnapshot();
    const bool selectionChanged
        = m_state.activeNavigationSourceKind() != ActiveNavigationSourceKind::OrdinaryDirectMedia
        || !previousSnapshot.known
        || previousSnapshot.currentNumber != result.boundaryState.currentNumber
        || previousSnapshot.count != result.boundaryState.count;
    m_state.setDirectMediaNavigation(result.boundaryState, true, result.candidates);
    qCDebug(kiriviewNavigationLog)
        << "direct media navigation refresh finished"
        << "candidates" << result.candidates.size() << "currentNumber"
        << result.boundaryState.currentNumber << "count" << result.boundaryState.count
        << "canPrevious" << result.boundaryState.canOpenPrevious << "canNext"
        << result.boundaryState.canOpenNext;
    if (selectionChanged) {
        setActiveNavigationRevealContext(takePendingActiveNavigationRevealContext(
            ActiveNavigationRevealIntent::ProgrammaticSync));
    }
    recomputePublicProjection();
    scheduleMediaPredecode(result.candidates);
}

void DocumentSessionRuntime::scheduleMediaPredecode(
    const std::vector<DirectMediaNavigationCandidate> &candidates)
{
    if (!directMediaNavigationActive() || m_mediaPredecodeCoordinator == nullptr) {
        return;
    }

    const QUrl currentUrl = activeDirectMediaCursorUrl();
    if (currentUrl.isEmpty()) {
        return;
    }

    m_mediaPredecodeCoordinator->schedule(MediaPredecodeCoordinator::Context {
        currentUrl,
        candidates,
        displayedPredecodeImages(),
        firstDisplayDecodeContext(),
    });
}

void DocumentSessionRuntime::cacheDisplayedMediaPredecodeImages()
{
    if (!directMediaNavigationActive() || m_mediaPredecodeCoordinator == nullptr) {
        return;
    }

    std::vector<DisplayedPredecodeImage> displayedImages = displayedPredecodeImages();
    if (displayedImages.empty()) {
        return;
    }

    m_mediaPredecodeCoordinator->cacheDisplayedImages(displayedImages);
}

std::vector<DisplayedPredecodeImage> DocumentSessionRuntime::displayedPredecodeImages() const
{
    if (m_state.documentKind() != DocumentSessionKind::Image
        || !activeImageUsesImageDocumentSourceScope() || !m_imageDocument.ready()) {
        return {};
    }

    std::optional<DisplayedPredecodeImage> displayed
        = m_imageDocument.primaryDisplayedPredecodeImage();
    if (!displayed.has_value()) {
        return {};
    }

    return { std::move(*displayed) };
}

ImageFirstDisplayDecodeContext DocumentSessionRuntime::firstDisplayDecodeContext() const
{
    return m_imageDocument.firstDisplayDecodeContext();
}

void DocumentSessionRuntime::cancelMediaDeletion()
{
    const bool sessionMediaDeletionInProgress
        = m_state.fileDeletionInProgress() && directMediaNavigationActive();
    if (!m_mediaDeletionRuntime.active() && !sessionMediaDeletionInProgress) {
        return;
    }

    m_mediaDeletionRuntime.cancel();
    m_directMediaDeletionCandidateRuntime.cancel();
    m_state.setFileDeletionInProgress(false);
    recomputePublicProjection();
}

void DocumentSessionRuntime::startMediaDeletion(
    FileDeletionMode mode, std::vector<DirectMediaNavigationCandidate> candidates)
{
    const DocumentSessionMediaDeletionStartPlan plan = m_mediaDeletionRuntime.start(m_owner, mode,
        std::move(candidates), activeDirectMediaCursorUrl(), m_state.documentKind(),
        [this](DocumentSessionMediaDeletionCompletion completion) {
            finishMediaDeletion(std::move(completion));
        });
    if (!plan.shouldStartDeletion) {
        m_state.setFileDeletionInProgress(false);
        recomputePublicProjection();
    }
}

MediaOpenWithPlan DocumentSessionRuntime::currentMediaOpenWithPlan() const
{
    return mediaOpenWithPlan(MediaOpenWithPlanInput {
        m_state.documentKind(),
        m_imageDocument.ready(),
        m_imageDocument.displayedUrl(),
        m_imageDocument.displayedOpenedCollectionScope(),
        m_videoDocument.ready(),
        m_videoDocument.sourceUrl(),
    });
}

void DocumentSessionRuntime::finishMediaDeletion(DocumentSessionMediaDeletionCompletion completion)
{
    m_state.setFileDeletionInProgress(false);
    recomputePublicProjection();

    executeMediaDeletionCompletionPlan(completion.plan, completion.errorString);
}

void DocumentSessionRuntime::executeMediaDeletionCompletionPlan(
    const DocumentSessionMediaDeletionCompletionPlan &plan, const QString &errorString)
{
    if (plan.reportFailure) {
        m_state.setSessionErrorString(
            errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString);
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
    return m_imageDocument.ordinaryDirectMediaScopeActive();
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
    const QUrl displayedUrl = m_imageDocument.displayedUrl();
    if (!pendingUrl.isEmpty()) {
        if (activeImageUsesImageDocumentSourceScope()
            && sameNormalizedUrl(displayedUrl, pendingUrl)) {
            return m_state.confirmDirectImageCursor(displayedUrl);
        }

        if (m_imageDocument.error()
            || (!m_imageDocument.sourceUrl().isEmpty()
                && m_imageDocument.sourceUrl() != pendingUrl)) {
            return m_state.restoreDirectImageCursorAfterFailure();
        }
        return false;
    }

    if (activeImageUsesImageDocumentSourceScope() && !displayedUrl.isEmpty()) {
        return m_state.confirmDirectImageCursor(displayedUrl);
    }

    if (m_imageDocument.error()) {
        return m_state.restoreDirectImageCursorAfterFailure();
    }

    return false;
}

ActiveZoomSnapshot DocumentSessionRuntime::activeZoomSnapshotForKind(DocumentSessionKind kind) const
{
    switch (kind) {
    case DocumentSessionKind::Image:
        if (!m_imageDocument.zoomPercentKnown()) {
            return {};
        }
        return ActiveZoomSnapshot { true, true, m_imageDocument.zoomPercent(), true };
    case DocumentSessionKind::Video:
        return ActiveZoomSnapshot {
            true,
            m_videoDocument.zoomPercentKnown(),
            m_videoDocument.zoomPercentKnown() ? qreal(m_videoDocument.zoomPercent()) : 0.0,
            false,
        };
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

DocumentSessionPublicProjectionInput DocumentSessionRuntime::publicProjectionInput() const
{
    return DocumentSessionPublicProjectionInput {
        m_state.documentKind(),
        directImageLoadMayUseImageDocumentSourceScope(),
        !m_imageDocument.sourceUrl().isEmpty()
            && !isSupportedDirectImageUrl(m_imageDocument.sourceUrl()),
        m_state.fileDeletionInProgress(),
        directMediaActiveNavigationInput(),
        imageDocumentPageActiveNavigationSnapshot(),
        m_imageDocument.windowTitleFileName(),
        m_imageDocument.primaryImageSize(),
        m_videoDocument.windowTitleFileName(),
        m_videoDocument.videoSize(),
        m_imageDocument.ready(),
        !m_state.directMediaCursor().pendingUrl.isEmpty(),
        !m_videoDocument.sourceUrl().isEmpty(),
        m_videoDocument.error(),
        currentMediaOpenWithPlan().hasRequest(),
    };
}

DirectMediaActiveNavigationInput DocumentSessionRuntime::directMediaActiveNavigationInput() const
{
    return DirectMediaActiveNavigationInput { m_state.directMediaNavigationState(),
        m_state.directMediaNavigationKnown() };
}

ImageDocumentPageActiveNavigationSnapshot
DocumentSessionRuntime::imageDocumentPageActiveNavigationSnapshot() const
{
    return m_imageDocument.activeNavigationSnapshot();
}
}
