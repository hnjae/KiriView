// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "localization/imageerrortext.h"
#include "location/imageurl.h"
#include "predecode/mediapredecodecoordinator.h"
#include "session/windowtitleprojection.h"

#include <QObject>
#include <QString>
#include <memory>
#include <optional>
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
    , m_mediaCandidateProvider(mediaNavigationCandidateProviderWithDefault(
          std::move(dependencies.mediaCandidateProvider)))
    , m_fileOperationProvider(
          fileOperationProviderWithDefault(std::move(dependencies.fileOperationProvider)))
    , m_mediaPredecodeCoordinator(std::make_unique<MediaPredecodeCoordinator>(owner,
          std::move(dependencies.imageDocumentDependencies.imageDecode),
          std::move(dependencies.imageDocumentDependencies.powerSaver)))
{
    connectDocuments();
}

DocumentSessionRuntime::~DocumentSessionRuntime()
{
    m_mediaCandidateJob.cancel();
    m_fileDeletionJob.cancel();
    m_fileDeletionOperation.cancel();
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
    if (fileDeletionInProgress()) {
        return false;
    }

    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        if (directImageLoadMayUseMediaScope()
            && !m_state.directMediaCursor().pendingUrl.isEmpty()) {
            return false;
        }
        return m_imageDocument.status() == KiriImageDocument::Status::Ready;
    case DocumentSessionKind::Video:
        return !m_videoDocument.sourceUrl().isEmpty()
            && m_videoDocument.status() != KiriVideoDocument::Status::Error;
    case DocumentSessionKind::Empty:
        return false;
    }

    return false;
}

bool DocumentSessionRuntime::fileDeletionInProgress() const
{
    return m_state.fileDeletionInProgress();
}

bool DocumentSessionRuntime::activeZoomPercentAvailable() const
{
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return m_imageDocument.zoomPercentKnown();
    case DocumentSessionKind::Video:
        return true;
    case DocumentSessionKind::Empty:
        return false;
    }

    return false;
}

bool DocumentSessionRuntime::activeZoomPercentKnown() const
{
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return m_imageDocument.zoomPercentKnown();
    case DocumentSessionKind::Video:
        return m_videoDocument.zoomPercentKnown();
    case DocumentSessionKind::Empty:
        return false;
    }

    return false;
}

qreal DocumentSessionRuntime::activeZoomPercent() const
{
    if (!activeZoomPercentKnown()) {
        return 0.0;
    }

    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return m_imageDocument.zoomPercent();
    case DocumentSessionKind::Video:
        return m_videoDocument.zoomPercent();
    case DocumentSessionKind::Empty:
        return 0.0;
    }

    return 0.0;
}

bool DocumentSessionRuntime::activeZoomEditable() const
{
    return m_state.documentKind() == DocumentSessionKind::Image
        && m_imageDocument.zoomPercentKnown();
}

bool DocumentSessionRuntime::mediaNavigationActive() const
{
    return m_state.documentKind() == DocumentSessionKind::Video
        || (m_state.documentKind() == DocumentSessionKind::Image
            && directImageLoadMayUseMediaScope());
}

bool DocumentSessionRuntime::mediaNavigationKnown() const
{
    return mediaNavigationActive() && m_state.mediaNavigationKnown();
}

int DocumentSessionRuntime::currentMediaNumber() const
{
    return mediaNavigationKnown() ? m_state.mediaNavigationState().currentNumber : 0;
}

int DocumentSessionRuntime::mediaCount() const
{
    return mediaNavigationKnown() ? m_state.mediaNavigationState().count : 0;
}

bool DocumentSessionRuntime::canOpenPreviousMedia() const
{
    return mediaNavigationActive() && m_state.mediaNavigationKnown()
        && m_state.mediaNavigationState().canOpenPrevious;
}

bool DocumentSessionRuntime::canOpenNextMedia() const
{
    return mediaNavigationActive() && m_state.mediaNavigationKnown()
        && m_state.mediaNavigationState().canOpenNext;
}

bool DocumentSessionRuntime::atKnownFirstMedia() const
{
    return mediaNavigationActive() && m_state.mediaNavigationKnown()
        && m_state.mediaNavigationState().atKnownFirst;
}

bool DocumentSessionRuntime::atKnownLastMedia() const
{
    return mediaNavigationActive() && m_state.mediaNavigationKnown()
        && m_state.mediaNavigationState().atKnownLast;
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
    return activeNavigationBoundaryScopeForSource(activeNavigationSourceKind());
}

std::optional<PredecodedImage> DocumentSessionRuntime::findPredecodedImage(const QUrl &url) const
{
    return m_mediaPredecodeCoordinator != nullptr
        ? m_mediaPredecodeCoordinator->findPredecodedImage(url)
        : std::optional<PredecodedImage>();
}

void DocumentSessionRuntime::openPreviousMedia()
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this](MediaCandidateLoadResult result) {
        finishMediaNavigation(std::move(result), previousMediaNavigationOpenRequest());
    });
}

void DocumentSessionRuntime::openNextMedia()
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this](MediaCandidateLoadResult result) {
        finishMediaNavigation(std::move(result), nextMediaNavigationOpenRequest());
    });
}

void DocumentSessionRuntime::openMediaAtNumber(int mediaNumber)
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this, mediaNumber](MediaCandidateLoadResult result) {
        finishMediaNavigation(std::move(result), numberedMediaNavigationOpenRequest(mediaNumber));
    });
}

void DocumentSessionRuntime::openPreviousActiveNavigation()
{
    if (!canOpenPreviousActiveNavigation()) {
        return;
    }

    switch (activeNavigationSourceKind()) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        openPreviousMedia();
        return;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        m_imageDocument.openPreviousImage();
        return;
    case ActiveNavigationSourceKind::None:
        return;
    }
}

void DocumentSessionRuntime::openNextActiveNavigation()
{
    if (!canOpenNextActiveNavigation()) {
        return;
    }

    switch (activeNavigationSourceKind()) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        openNextMedia();
        return;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        m_imageDocument.openNextImage();
        return;
    case ActiveNavigationSourceKind::None:
        return;
    }
}

void DocumentSessionRuntime::openFirstActiveNavigation()
{
    if (!activeNavigationKnown() || atKnownFirstActiveNavigation()) {
        return;
    }

    openActiveNavigationAtNumber(1);
}

void DocumentSessionRuntime::openLastActiveNavigation()
{
    const ActiveNavigationSnapshot snapshot = m_state.activeNavigationSnapshot();
    if (!snapshot.known || snapshot.atKnownLast) {
        return;
    }

    openActiveNavigationAtNumber(snapshot.count);
}

void DocumentSessionRuntime::openActiveNavigationAtNumber(int number)
{
    const ActiveNavigationSnapshot snapshot = m_state.activeNavigationSnapshot();
    if (!snapshot.known || !snapshot.editable) {
        return;
    }

    switch (activeNavigationSourceKind()) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        openMediaAtNumber(number);
        return;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        m_imageDocument.openImageAtPage(number);
        return;
    case ActiveNavigationSourceKind::None:
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
    recomputeActiveNavigation();
    loadMediaCandidates([this, mode](MediaCandidateLoadResult result) mutable {
        startMediaDeletion(mode, std::move(result.candidates));
    });
}

void DocumentSessionRuntime::connectDocuments()
{
    QObject::connect(&m_imageDocument, &KiriImageDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::statusChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::windowTitleFileNameChanged, m_owner,
        [this]() { recomputeWindowTitleSubject(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::imageSizeChanged, m_owner,
        [this]() { recomputeWindowTitleSubject(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::documentScopeChanged, m_owner, [this]() {
        if (m_routingSource) {
            return;
        }

        const bool directMediaScopeChanged = syncDirectImageCursorFromDocument();
        if (directMediaScopeChanged || !mediaNavigationActive()) {
            refreshMediaNavigation();
        }
        recomputeActiveNavigation();
        m_state.publish(DocumentSessionChange::FileDeletionAvailability);
    });
    QObject::connect(&m_imageDocument, &KiriImageDocument::fileDeletionInProgressChanged, m_owner,
        [this]() { syncImageDocumentFileDeletionProgress(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::zoomPercentKnownChanged, m_owner,
        [this]() { publishActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::zoomPercentChanged, m_owner,
        [this]() { publishActiveZoomReadoutForKind(DocumentSessionKind::Image); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::pageNavigationChanged, m_owner,
        [this]() { publishActiveNavigationForImagePages(); });

    QObject::connect(&m_videoDocument, &KiriVideoDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::statusChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::windowTitleFileNameChanged, m_owner,
        [this]() { recomputeWindowTitleSubject(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::videoSizeChanged, m_owner,
        [this]() { recomputeWindowTitleSubject(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::zoomPercentKnownChanged, m_owner,
        [this]() { publishActiveZoomReadoutForKind(DocumentSessionKind::Video); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::zoomPercentChanged, m_owner,
        [this]() { publishActiveZoomReadoutForKind(DocumentSessionKind::Video); });
}

void DocumentSessionRuntime::syncImageDocumentFileDeletionProgress()
{
    if (m_state.documentKind() != DocumentSessionKind::Image || directImageLoadMayUseMediaScope()) {
        return;
    }

    m_state.setFileDeletionInProgress(m_imageDocument.fileDeletionInProgress());
    recomputeActiveNavigation();
}

void DocumentSessionRuntime::publishActiveZoomReadoutForKind(DocumentSessionKind kind)
{
    if (m_state.documentKind() == kind) {
        m_state.publish(DocumentSessionChange::ActiveZoomReadout);
    }
}

void DocumentSessionRuntime::publishActiveNavigationForImagePages()
{
    if (activeNavigationSourceKind() == ActiveNavigationSourceKind::ImageDocumentPages) {
        recomputeActiveNavigation();
    }
}

void DocumentSessionRuntime::recomputeActiveNavigation()
{
    m_state.setActiveNavigationSnapshot(projectedActiveNavigationSnapshot());
    recomputeWindowTitleSubject();
}

void DocumentSessionRuntime::recomputeWindowTitleSubject()
{
    m_state.setWindowTitleSubject(projectedWindowTitleSubject());
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    const DocumentSessionRoutePlan plan
        = documentSessionRoutePlanForSourceUrl(sourceUrl, m_state.documentKind());
    m_state.setSessionErrorString(QString());
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();
    cancelMediaDeletion();

    executeRoutePlan(plan);
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    executeRoutePlan(documentSessionRoutePlanForMediaUrl(url, m_state.documentKind()));
}

void DocumentSessionRuntime::executeRoutePlan(const DocumentSessionRoutePlan &plan)
{
    if (plan.clearMediaNavigation) {
        m_state.setMediaNavigationState({}, false);
    }

    bool directMediaScopeChanged = false;
    switch (plan.cursorAction) {
    case DocumentSessionRouteCursorAction::None:
        break;
    case DocumentSessionRouteCursorAction::Clear:
        directMediaScopeChanged = m_state.clearDirectMediaCursor();
        break;
    case DocumentSessionRouteCursorAction::SetDirectVideo:
        directMediaScopeChanged = m_state.setDirectVideoCursor(plan.sourceUrl);
        break;
    case DocumentSessionRouteCursorAction::RequestDirectImage:
        directMediaScopeChanged = m_state.requestDirectImageCursor(plan.sourceUrl);
        break;
    case DocumentSessionRouteCursorAction::ClearThenRequestDirectImage:
        directMediaScopeChanged = m_state.clearDirectMediaCursor();
        directMediaScopeChanged
            = m_state.requestDirectImageCursor(plan.sourceUrl) || directMediaScopeChanged;
        break;
    }

    m_routingSource = true;
    if (plan.clearVideo) {
        leaveVideoMode();
    }
    if (plan.clearImageDocument) {
        m_imageDocument.setSourceUrl(QUrl());
    }
    if (plan.enterVideo) {
        m_videoDocument.setSourceUrl(plan.sourceUrl);
        m_state.setDocumentKind(DocumentSessionKind::Video);
    }
    if (plan.enterImage) {
        m_imageDocument.setSourceUrl(plan.sourceUrl);
        m_state.setDocumentKind(DocumentSessionKind::Image);
    }
    if (plan.enterEmpty) {
        m_state.setDocumentKind(DocumentSessionKind::Empty);
    }
    m_routingSource = false;

    if (plan.syncDirectImageCursorFromDocument) {
        directMediaScopeChanged = syncDirectImageCursorFromDocument() || directMediaScopeChanged;
    }

    switch (plan.sourceIdentityAction) {
    case DocumentSessionRouteSourceIdentityAction::None:
        break;
    case DocumentSessionRouteSourceIdentityAction::Clear:
        m_state.setSourceIdentity(QUrl());
        break;
    case DocumentSessionRouteSourceIdentityAction::UseOriginalUrl:
        m_state.setSourceIdentity(plan.sourceUrl);
        break;
    case DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl:
        m_state.setSourceIdentity(m_imageDocument.sourceUrl());
        break;
    }

    recomputeActiveNavigation();

    if (plan.refreshMediaNavigation
        && (directMediaScopeChanged || plan.clearMediaNavigation || mediaNavigationActive())) {
        refreshMediaNavigation();
    }
    if (plan.clearPredecode) {
        if (plan.clearMediaNavigation) {
            m_state.setMediaNavigationState({}, false);
            recomputeActiveNavigation();
        }
        if (m_mediaPredecodeCoordinator != nullptr) {
            m_mediaPredecodeCoordinator->clear();
        }
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
    recomputeActiveNavigation();
    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability });
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
        m_state.setDocumentKind(DocumentSessionKind::Empty);
        m_state.setMediaNavigationState({}, false);
    } else {
        m_state.setDirectVideoCursor(m_videoDocument.sourceUrl());
        m_state.setSourceIdentity(m_videoDocument.sourceUrl());
        refreshMediaNavigation();
    }

    recomputeActiveNavigation();
    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability });
}

void DocumentSessionRuntime::refreshMediaNavigation()
{
    if (!mediaNavigationActive()) {
        m_state.setMediaNavigationState({}, false);
        recomputeActiveNavigation();
        if (!directImageLoadMayUseMediaScope()) {
            m_mediaPredecodeCoordinator->clear();
        }
        return;
    }

    loadMediaCandidates(
        [this](MediaCandidateLoadResult result) { updateMediaBoundaryState(std::move(result)); });
}

void DocumentSessionRuntime::loadMediaCandidates(
    std::function<void(MediaCandidateLoadResult)> callback)
{
    const QUrl currentUrl = activeDirectMediaCursorUrl();
    const QUrl parentUrl = activeDirectMediaScopeUrl();
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();

    if (currentUrl.isEmpty() || parentUrl.isEmpty() || !parentUrl.isValid()
        || !m_mediaCandidateProvider.directoryMedia) {
        callback(MediaCandidateLoadResult {});
        return;
    }

    const DocumentSessionMediaCandidateLoad load = m_mediaCandidateLoadState.start(
        currentUrl, parentUrl, m_state.directMediaCursor().generation);
    auto sharedCallback
        = std::make_shared<std::function<void(MediaCandidateLoadResult)>>(std::move(callback));
    m_mediaCandidateJob = m_mediaCandidateProvider.directoryMedia(
        m_owner, parentUrl,
        [this, load, sharedCallback](std::vector<MediaNavigationCandidate> candidates) mutable {
            finishMediaCandidateLoad(load,
                MediaCandidateLoadResult { std::move(candidates), true, QString() },
                sharedCallback);
        },
        [this, load, sharedCallback](const QString &errorString) mutable {
            finishMediaCandidateLoad(
                load, MediaCandidateLoadResult { {}, false, errorString }, sharedCallback);
        });
}

void DocumentSessionRuntime::finishMediaCandidateLoad(DocumentSessionMediaCandidateLoad load,
    MediaCandidateLoadResult result,
    const std::shared_ptr<std::function<void(MediaCandidateLoadResult)>> &callback)
{
    if (!m_mediaCandidateLoadState.finish(load) || !directMediaCursorMatches(load) || !callback
        || !*callback) {
        return;
    }

    (*callback)(std::move(result));
}

void DocumentSessionRuntime::finishMediaNavigation(
    MediaCandidateLoadResult result, MediaNavigationOpenRequest request)
{
    if (!result.succeeded) {
        m_state.setMediaNavigationState({}, false);
        recomputeActiveNavigation();
        return;
    }

    const MediaNavigationOpenPlan plan
        = mediaNavigationOpenPlan(result.candidates, activeDirectMediaCursorUrl(), request);
    m_state.setMediaNavigationState(plan.boundaryState, true);
    recomputeActiveNavigation();
    scheduleMediaPredecode(result.candidates);
    if (plan.targetUrl.has_value()) {
        openMediaUrl(*plan.targetUrl);
    }
}

void DocumentSessionRuntime::updateMediaBoundaryState(MediaCandidateLoadResult result)
{
    if (!result.succeeded) {
        m_state.setMediaNavigationState({}, false);
        recomputeActiveNavigation();
        return;
    }

    m_state.setMediaNavigationState(
        mediaNavigationBoundaryState(result.candidates, activeDirectMediaCursorUrl()), true);
    recomputeActiveNavigation();
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
    if (!m_fileDeletionOperation.active() && !sessionMediaDeletionInProgress) {
        return;
    }

    m_fileDeletionJob.cancel();
    m_fileDeletionOperation.cancel();
    m_state.setFileDeletionInProgress(false);
    recomputeActiveNavigation();
}

void DocumentSessionRuntime::startMediaDeletion(
    FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates)
{
    const DocumentSessionMediaDeletionStartPlan plan = documentSessionMediaDeletionStartPlan(
        mode, std::move(candidates), activeDirectMediaCursorUrl());
    if (!plan.shouldStartDeletion) {
        m_state.setFileDeletionInProgress(false);
        recomputeActiveNavigation();
        return;
    }

    m_fileDeletionJob.cancel();
    const quint64 operationId = m_fileDeletionOperation.start();
    m_fileDeletionJob = m_fileOperationProvider(m_owner, plan.request,
        [this, operationId, fallbackPlan = plan.fallbackPlan](
            FileDeletionResult result, const QString &errorString) {
            finishMediaDeletion(operationId, fallbackPlan, result, errorString);
        });
}

void DocumentSessionRuntime::finishMediaDeletion(quint64 operationId,
    const MediaDeletionFallbackPlan &fallbackPlan, FileDeletionResult result,
    const QString &errorString)
{
    if (!m_fileDeletionOperation.finish(operationId)) {
        return;
    }

    m_state.setFileDeletionInProgress(false);
    recomputeActiveNavigation();

    executeMediaDeletionCompletionPlan(
        documentSessionMediaDeletionCompletionPlan(m_state.documentKind(), fallbackPlan, result),
        errorString);
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
        m_state.setDocumentKind(DocumentSessionKind::Empty);
        if (plan.clearMediaNavigation) {
            m_state.setMediaNavigationState({}, false);
            recomputeActiveNavigation();
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

QUrl DocumentSessionRuntime::activeDirectMediaCursorUrl() const
{
    return m_state.directMediaCursorUrl();
}

QUrl DocumentSessionRuntime::activeDirectMediaScopeUrl() const
{
    return mediaNavigationParentUrl(activeDirectMediaCursorUrl());
}

bool DocumentSessionRuntime::directMediaCursorMatches(
    const DocumentSessionMediaCandidateLoad &load) const
{
    return sameNormalizedUrl(activeDirectMediaCursorUrl(), load.currentUrl)
        && sameNormalizedUrl(activeDirectMediaScopeUrl(), load.parentUrl)
        && m_state.directMediaCursor().generation == load.cursorGeneration;
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

ActiveNavigationSourceKind DocumentSessionRuntime::activeNavigationSourceKind() const
{
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Video:
        return ActiveNavigationSourceKind::OrdinaryDirectMedia;
    case DocumentSessionKind::Image:
        if (directImageLoadMayUseMediaScope()) {
            return ActiveNavigationSourceKind::OrdinaryDirectMedia;
        }
        if (m_imageDocument.currentPageNumber() > 0 || m_imageDocument.imageCount() > 0
            || (!m_imageDocument.sourceUrl().isEmpty()
                && !isDocumentSessionDirectImageUrl(m_imageDocument.sourceUrl()))) {
            return ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return ActiveNavigationSourceKind::None;
    case DocumentSessionKind::Empty:
        return ActiveNavigationSourceKind::None;
    }

    return ActiveNavigationSourceKind::None;
}

ActiveNavigationSnapshot DocumentSessionRuntime::projectedActiveNavigationSnapshot() const
{
    return projectActiveNavigation(activeNavigationSourceKind(), mediaActiveNavigationInput(),
        imageDocumentActiveNavigationInput(), m_state.fileDeletionInProgress());
}

QString DocumentSessionRuntime::projectedWindowTitleSubject() const
{
    const ActiveNavigationSourceKind sourceKind = activeNavigationSourceKind();
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return projectWindowTitleSubject(WindowTitleSubjectInput {
            m_imageDocument.windowTitleFileName(),
            sourceKind,
            directMediaWindowTitleSizeForKind(DocumentSessionKind::Image, sourceKind),
            m_state.activeNavigationSnapshot(),
        });
    case DocumentSessionKind::Video:
        return projectWindowTitleSubject(WindowTitleSubjectInput {
            m_videoDocument.windowTitleFileName(),
            sourceKind,
            directMediaWindowTitleSizeForKind(DocumentSessionKind::Video, sourceKind),
            m_state.activeNavigationSnapshot(),
        });
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

QSize DocumentSessionRuntime::directMediaWindowTitleSizeForKind(
    DocumentSessionKind kind, ActiveNavigationSourceKind sourceKind) const
{
    if (sourceKind != ActiveNavigationSourceKind::OrdinaryDirectMedia) {
        return {};
    }

    switch (kind) {
    case DocumentSessionKind::Image:
        return m_imageDocument.primaryImageSize();
    case DocumentSessionKind::Video:
        return m_videoDocument.videoSize();
    case DocumentSessionKind::Empty:
        return {};
    }

    return {};
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
