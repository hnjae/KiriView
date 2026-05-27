// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "localization/imageerrortext.h"
#include "location/imageurl.h"
#include "navigation/mediaformatregistry.h"
#include "predecode/mediapredecodecoordinator.h"
#include "predecode/predecodecachebudget.h"
#include "session/activenavigationthumbnailprojection.h"
#include "session/mediaopenwithtarget.h"

#include <QAbstractListModel>
#include <QObject>
#include <QScopedValueRollback>
#include <QString>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace {
QString genericFileDeletionErrorMessage()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DeleteFile);
}
}

namespace KiriView {
DocumentSessionRuntime::DocumentSessionRuntime(QObject *owner, KiriImageDocument &imageDocument,
    KiriVideoDocument &videoDocument, ChangeCallback changeCallback,
    DocumentSessionRuntimeDependencies dependencies)
    : m_owner(owner)
    , m_imageDocument(imageDocument)
    , m_videoDocument(videoDocument)
    , m_state(std::move(changeCallback))
    , m_activeNavigationThumbnailModel(std::make_unique<ActiveNavigationThumbnailModel>(owner))
    , m_mediaNavigationRuntime(std::move(dependencies.mediaCandidateProvider))
    , m_mediaDeletionRuntime(std::move(dependencies.fileOperationProvider))
    , m_mediaOpenWithProvider(
          mediaOpenWithProviderWithDefault(std::move(dependencies.mediaOpenWithProvider)))
    , m_mediaPredecodeCoordinator(std::make_unique<MediaPredecodeCoordinator>(owner,
          std::move(dependencies.imageDocumentDependencies.imageDecode),
          std::move(dependencies.imageDocumentDependencies.powerSaver),
          KiriView::resolvedPredecodeCacheByteBudget(
              dependencies.imageDocumentDependencies.predecodeCacheByteBudget)))
{
    connectDocuments();
}

DocumentSessionRuntime::~DocumentSessionRuntime()
{
    m_mediaNavigationRuntime.cancel();
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

bool DocumentSessionRuntime::mediaNavigationActive() const
{
    return m_state.documentKind() == DocumentSessionKind::Video
        || (m_state.documentKind() == DocumentSessionKind::Image
            && directImageLoadMayUseMediaScope());
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
    openMedia(previousMediaNavigationOpenRequest());
}

void DocumentSessionRuntime::openNextMedia() { openMedia(nextMediaNavigationOpenRequest()); }

void DocumentSessionRuntime::openMediaAtNumber(int mediaNumber)
{
    openMedia(numberedMediaNavigationOpenRequest(mediaNumber));
}

void DocumentSessionRuntime::openMedia(MediaNavigationOpenRequest request)
{
    if (!mediaNavigationActive()) {
        return;
    }

    m_mediaNavigationRuntime.open(
        m_owner, mediaNavigationLoadScope(), request,
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        [this](DocumentSessionMediaNavigationOpenResult result) {
            finishMediaNavigation(std::move(result));
        });
}

void DocumentSessionRuntime::openPreviousActiveNavigation() { requestPreviousActiveNavigation(); }

void DocumentSessionRuntime::openNextActiveNavigation() { requestNextActiveNavigation(); }

void DocumentSessionRuntime::openFirstActiveNavigation()
{
    executeActiveNavigationDispatchRequest(firstActiveNavigationDispatchRequest());
}

void DocumentSessionRuntime::openLastActiveNavigation()
{
    executeActiveNavigationDispatchRequest(lastActiveNavigationDispatchRequest());
}

void DocumentSessionRuntime::openActiveNavigationAtNumber(int number)
{
    executeActiveNavigationDispatchRequest(numberedActiveNavigationDispatchRequest(number));
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::requestPreviousActiveNavigation()
{
    return executeActiveNavigationDispatchRequest(previousActiveNavigationDispatchRequest());
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::requestNextActiveNavigation()
{
    return executeActiveNavigationDispatchRequest(nextActiveNavigationDispatchRequest());
}

ActiveNavigationDispatchOutcome DocumentSessionRuntime::executeActiveNavigationDispatchRequest(
    ActiveNavigationDispatchRequest request)
{
    const ActiveNavigationDispatchPlan plan = activeNavigationDispatchPlan(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(), request);
    executeActiveNavigationDispatchPlan(plan);
    return plan.outcome;
}

void DocumentSessionRuntime::executeActiveNavigationDispatchPlan(
    const ActiveNavigationDispatchPlan &plan)
{
    if (!plan.shouldDispatch()) {
        return;
    }

    switch (plan.target) {
    case ActiveNavigationDispatchTarget::OrdinaryDirectMedia:
        switch (plan.operation) {
        case ActiveNavigationDispatchOperation::OpenPrevious:
            openPreviousMedia();
            return;
        case ActiveNavigationDispatchOperation::OpenNext:
            openNextMedia();
            return;
        case ActiveNavigationDispatchOperation::OpenNumber:
            openMediaAtNumber(plan.number);
            return;
        case ActiveNavigationDispatchOperation::None:
            return;
        }
        return;
    case ActiveNavigationDispatchTarget::ImageDocumentPages:
        switch (plan.operation) {
        case ActiveNavigationDispatchOperation::OpenPrevious:
            m_imageDocument.openPreviousImage();
            return;
        case ActiveNavigationDispatchOperation::OpenNext:
            m_imageDocument.openNextImage();
            return;
        case ActiveNavigationDispatchOperation::OpenNumber:
            m_imageDocument.openImageAtPage(plan.number);
            return;
        case ActiveNavigationDispatchOperation::None:
            return;
        }
        return;
    case ActiveNavigationDispatchTarget::None:
        return;
    }
}

void DocumentSessionRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_state.documentKind() == DocumentSessionKind::Image
        && !directImageLoadMayUseMediaScope()) {
        m_imageDocument.deleteDisplayedFile(mode == FileDeletionMode::MoveToTrash
                ? KiriImageDocument::DeletionMode::MoveToTrash
                : KiriImageDocument::DeletionMode::DeletePermanently);
        syncImageDocumentFileDeletionProgress();
        return;
    }

    if (!displayedFileDeletionAvailable() || !mediaNavigationActive()) {
        return;
    }

    m_state.setFileDeletionInProgress(true);
    recomputePublicProjection();
    loadMediaCandidates(
        [this, mode](DocumentSessionMediaNavigationCandidatesResult result) mutable {
            startMediaDeletion(mode, std::move(result.candidates));
        });
}

void DocumentSessionRuntime::openCurrentMediaWith(MediaOpenWithCallback callback)
{
    const QUrl targetUrl = currentMediaOpenWithTargetUrl();
    if (targetUrl.isEmpty()) {
        if (callback) {
            callback(MediaOpenWithResult::Failed, QString());
        }
        return;
    }

    m_mediaOpenWithJob
        = m_mediaOpenWithProvider(m_owner, MediaOpenWithRequest { targetUrl }, std::move(callback));
}

void DocumentSessionRuntime::connectDocuments()
{
    QObject::connect(&m_imageDocument, &KiriImageDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::statusChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::windowTitleFileNameChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::imageSizeChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::mediaScopeChanged, m_owner, [this]() {
        if (m_routingSource) {
            return;
        }

        const bool directMediaScopeChanged = syncDirectImageCursorFromDocument();
        if (directMediaScopeChanged || !mediaNavigationActive()) {
            refreshMediaNavigation();
        }
        recomputePublicProjection();
    });
    QObject::connect(&m_imageDocument, &KiriImageDocument::fileDeletionInProgressChanged, m_owner,
        [this]() { syncImageDocumentFileDeletionProgress(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::zoomPercentKnownChanged, m_owner,
        [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::zoomPercentChanged, m_owner,
        [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::pageNavigationChanged, m_owner,
        [this]() { publishActiveNavigationForImagePages(); });

    QObject::connect(&m_videoDocument, &KiriVideoDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::statusChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::windowTitleFileNameChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::videoSizeChanged, m_owner,
        [this]() { recomputePublicProjection(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::zoomPercentKnownChanged, m_owner,
        [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Video); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::zoomPercentChanged, m_owner,
        [this]() { recomputeActiveZoomReadoutForKind(DocumentSessionKind::Video); });
}

void DocumentSessionRuntime::syncImageDocumentFileDeletionProgress()
{
    if (m_state.documentKind() != DocumentSessionKind::Image || directImageLoadMayUseMediaScope()) {
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
    if (m_state.updatePublicProjectionForSourceKind(
            publicProjectionInput(), ActiveNavigationSourceKind::ImageDocumentPages)) {
        syncActiveNavigationThumbnailRows();
    }
}

void DocumentSessionRuntime::recomputePublicProjection()
{
    m_state.updatePublicProjection(publicProjectionInput());
    syncActiveNavigationThumbnailRows();
}

void DocumentSessionRuntime::syncActiveNavigationThumbnailRows()
{
    if (m_activeNavigationThumbnailModel == nullptr) {
        return;
    }

    std::vector<ActiveNavigationThumbnailRow> rows = projectActiveNavigationThumbnailRows(
        m_state.activeNavigationSourceKind(), m_state.activeNavigationSnapshot(),
        m_mediaNavigationCandidates, m_imageDocument.pageNavigationSnapshot());
    if (rows.empty()) {
        m_activeNavigationThumbnailModel->clear();
        return;
    }

    m_activeNavigationThumbnailModel->setRows(std::move(rows));
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    const DocumentSessionRoutePlan plan
        = documentSessionRoutePlanForSourceUrl(sourceUrl, m_state.documentKind());
    executeRoutePlan(plan);
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    executeRoutePlan(documentSessionRoutePlanForMediaUrl(url, m_state.documentKind()));
}

void DocumentSessionRuntime::executeRoutePlan(const DocumentSessionRoutePlan &plan)
{
    struct RouteExecutionState {
        bool directMediaScopeChanged = false;
        bool mediaNavigationCleared = false;
    };

    RouteExecutionState state;
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
                                         CancelMediaNavigationRouteOperation>) {
                    m_mediaNavigationRuntime.cancel();
                } else if constexpr (std::is_same_v<Operation, CancelMediaDeletionRouteOperation>) {
                    cancelMediaDeletion();
                } else if constexpr (std::is_same_v<Operation,
                                         ClearMediaNavigationRouteOperation>) {
                    m_state.setMediaNavigationState({}, false);
                    m_mediaNavigationCandidates.clear();
                    state.mediaNavigationCleared = true;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearDirectMediaCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = m_state.clearDirectMediaCursor() || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         SetDirectVideoCursorRouteOperation>) {
                    state.directMediaScopeChanged = m_state.setDirectVideoCursor(payload.url)
                        || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         RequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged = m_state.requestDirectImageCursor(payload.url)
                        || state.directMediaScopeChanged;
                } else if constexpr (std::is_same_v<Operation,
                                         ClearThenRequestDirectImageCursorRouteOperation>) {
                    state.directMediaScopeChanged
                        = m_state.clearDirectMediaCursor() || state.directMediaScopeChanged;
                    state.directMediaScopeChanged = m_state.requestDirectImageCursor(payload.url)
                        || state.directMediaScopeChanged;
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
                                         RefreshMediaNavigationAfterRoutingRouteOperation>) {
                    if (state.directMediaScopeChanged || state.mediaNavigationCleared
                        || mediaNavigationActive()) {
                        refreshMediaNavigation();
                    }
                } else if constexpr (std::is_same_v<Operation, ClearMediaPredecodeRouteOperation>) {
                    if (state.mediaNavigationCleared) {
                        m_state.setMediaNavigationState({}, false);
                        recomputePublicProjection();
                    }
                    if (m_mediaPredecodeCoordinator != nullptr) {
                        m_mediaPredecodeCoordinator->clear();
                    }
                }
            },
            operation);
    }
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
        refreshMediaNavigation();
    }
}

void DocumentSessionRuntime::syncFromVideoDocument()
{
    if (m_routingSource || m_state.documentKind() != DocumentSessionKind::Video) {
        return;
    }

    if (m_videoDocument.sourceUrl().isEmpty()) {
        m_state.clearDirectMediaCursor();
        m_state.setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        m_state.setMediaNavigationState({}, false);
        m_mediaNavigationCandidates.clear();
    } else {
        m_state.setDirectVideoCursor(m_videoDocument.sourceUrl());
        m_state.setSourceIdentity(m_videoDocument.sourceUrl());
        refreshMediaNavigation();
    }

    recomputePublicProjection();
    recomputeActiveZoomReadout();
    m_state.publish(DocumentSessionChange::ErrorString);
}

void DocumentSessionRuntime::refreshMediaNavigation()
{
    if (!mediaNavigationActive()) {
        m_state.setMediaNavigationState({}, false);
        m_mediaNavigationCandidates.clear();
        recomputePublicProjection();
        if (!directImageLoadMayUseMediaScope()) {
            m_mediaPredecodeCoordinator->clear();
        }
        return;
    }

    m_mediaNavigationRuntime.refresh(
        m_owner, mediaNavigationLoadScope(),
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        [this](DocumentSessionMediaNavigationRefreshResult result) {
            updateMediaBoundaryState(std::move(result));
        });
}

void DocumentSessionRuntime::loadMediaCandidates(
    DocumentSessionMediaNavigationRuntime::CandidatesCallback callback)
{
    m_mediaNavigationRuntime.loadCandidates(
        m_owner, mediaNavigationLoadScope(),
        [this](const DirectMediaScope &scope) { return directMediaCursorMatches(scope); },
        std::move(callback));
}

void DocumentSessionRuntime::finishMediaNavigation(DocumentSessionMediaNavigationOpenResult result)
{
    if (!result.succeeded) {
        m_state.setMediaNavigationState({}, false);
        m_mediaNavigationCandidates.clear();
        recomputePublicProjection();
        return;
    }

    m_mediaNavigationCandidates = result.candidates;
    m_state.setMediaNavigationState(result.plan.boundaryState, true);
    recomputePublicProjection();
    scheduleMediaPredecode(result.candidates);
    if (result.plan.targetUrl.has_value()) {
        openMediaUrl(*result.plan.targetUrl);
    }
}

void DocumentSessionRuntime::updateMediaBoundaryState(
    DocumentSessionMediaNavigationRefreshResult result)
{
    if (!result.succeeded) {
        m_state.setMediaNavigationState({}, false);
        m_mediaNavigationCandidates.clear();
        recomputePublicProjection();
        return;
    }

    m_mediaNavigationCandidates = result.candidates;
    m_state.setMediaNavigationState(result.boundaryState, true);
    recomputePublicProjection();
    scheduleMediaPredecode(result.candidates);
}

void DocumentSessionRuntime::scheduleMediaPredecode(
    const std::vector<MediaNavigationCandidate> &candidates)
{
    if (!mediaNavigationActive() || m_mediaPredecodeCoordinator == nullptr) {
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

std::vector<DisplayedPredecodeImage> DocumentSessionRuntime::displayedPredecodeImages() const
{
    if (m_state.documentKind() != DocumentSessionKind::Image || !activeImageUsesMediaScope()
        || m_imageDocument.status() != KiriImageDocument::Status::Ready) {
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
        = m_state.fileDeletionInProgress() && mediaNavigationActive();
    if (!m_mediaDeletionRuntime.active() && !sessionMediaDeletionInProgress) {
        return;
    }

    m_mediaDeletionRuntime.cancel();
    m_state.setFileDeletionInProgress(false);
    recomputePublicProjection();
}

void DocumentSessionRuntime::startMediaDeletion(
    FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates)
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

QUrl DocumentSessionRuntime::currentMediaOpenWithTargetUrl() const
{
    return mediaOpenWithTargetUrl(MediaOpenWithTargetInput {
        m_state.documentKind(),
        m_imageDocument.status() == KiriImageDocument::Status::Ready,
        m_imageDocument.displayedUrl(),
        m_imageDocument.displayedImagePageScope(),
        m_videoDocument.status() == KiriVideoDocument::Status::Ready,
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
    switch (plan.clearDocument) {
    case DocumentSessionMediaDeletionDocumentClear::None:
        break;
    case DocumentSessionMediaDeletionDocumentClear::Image:
        m_imageDocument.setSourceUrl(QUrl());
        break;
    case DocumentSessionMediaDeletionDocumentClear::Video:
        leaveVideoMode();
        break;
    }

    switch (plan.followUp) {
    case DocumentSessionMediaDeletionFollowUp::None:
        return;
    case DocumentSessionMediaDeletionFollowUp::OpenFallback:
        if (!plan.fallbackUrl.isEmpty()) {
            openMediaUrl(plan.fallbackUrl);
        }
        return;
    case DocumentSessionMediaDeletionFollowUp::ClearSession:
        m_state.setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        if (plan.clearMediaNavigation) {
            m_state.setMediaNavigationState({}, false);
            m_mediaNavigationCandidates.clear();
            recomputePublicProjection();
        }
        if (plan.clearPredecode && m_mediaPredecodeCoordinator != nullptr) {
            m_mediaPredecodeCoordinator->clear();
        }
        return;
    case DocumentSessionMediaDeletionFollowUp::ReportFailure:
        m_state.setSessionErrorString(
            errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString);
        return;
    }
}

DirectMediaScope DocumentSessionRuntime::mediaNavigationLoadScope() const
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

bool DocumentSessionRuntime::activeImageUsesMediaScope() const
{
    return m_imageDocument.ordinaryDirectMediaScopeActive();
}

bool DocumentSessionRuntime::directImageLoadMayUseMediaScope() const
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
        if (activeImageUsesMediaScope() && sameNormalizedUrl(displayedUrl, pendingUrl)) {
            return m_state.confirmDirectImageCursor(displayedUrl);
        }

        if (m_imageDocument.status() == KiriImageDocument::Status::Error
            || (!m_imageDocument.sourceUrl().isEmpty()
                && m_imageDocument.sourceUrl() != pendingUrl)) {
            return m_state.restoreDirectImageCursorAfterFailure();
        }
        return false;
    }

    if (activeImageUsesMediaScope() && !displayedUrl.isEmpty()) {
        return m_state.confirmDirectImageCursor(displayedUrl);
    }

    if (m_imageDocument.status() == KiriImageDocument::Status::Error) {
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
        directImageLoadMayUseMediaScope(),
        !m_imageDocument.sourceUrl().isEmpty()
            && !isSupportedDirectImageUrl(m_imageDocument.sourceUrl()),
        m_state.fileDeletionInProgress(),
        mediaActiveNavigationInput(),
        imageDocumentActiveNavigationInput(),
        m_imageDocument.windowTitleFileName(),
        m_imageDocument.primaryImageSize(),
        m_videoDocument.windowTitleFileName(),
        m_videoDocument.videoSize(),
        m_imageDocument.status() == KiriImageDocument::Status::Ready,
        !m_state.directMediaCursor().pendingUrl.isEmpty(),
        !m_videoDocument.sourceUrl().isEmpty(),
        m_videoDocument.status() == KiriVideoDocument::Status::Error,
        !currentMediaOpenWithTargetUrl().isEmpty(),
    };
}

MediaActiveNavigationInput DocumentSessionRuntime::mediaActiveNavigationInput() const
{
    return MediaActiveNavigationInput { m_state.mediaNavigationState(),
        m_state.mediaNavigationKnown() };
}

ImageDocumentActiveNavigationInput
DocumentSessionRuntime::imageDocumentActiveNavigationInput() const
{
    return ImageDocumentActiveNavigationInput { m_imageDocument.currentPageNumber(),
        m_imageDocument.currentLastPageNumber(), m_imageDocument.imageCount() };
}
}
