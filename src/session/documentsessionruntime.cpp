// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "decoding/imageformatregistry.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "localization/imageerrortext.h"
#include "navigation/mediaformatregistry.h"
#include "predecode/mediapredecodecoordinator.h"

#include <QObject>
#include <QString>
#include <memory>
#include <optional>
#include <utility>

namespace {
bool isDirectVideoUrl(const QUrl &url)
{
    const QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (KiriView::isSupportedDirectVideoFileName(fileName)) {
        return true;
    }

    return KiriView::isSupportedDirectVideoFileName(url.toString(QUrl::PrettyDecoded));
}

bool isDirectImageUrl(const QUrl &url)
{
    const QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (KiriView::isSupportedImageFileName(fileName)) {
        return true;
    }

    return KiriView::isSupportedImageFileName(url.toString(QUrl::PrettyDecoded));
}

QString genericFileDeletionErrorMessage()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::DeleteFile);
}

KiriView::ActiveNavigationSnapshot unavailableActiveNavigation() { return {}; }

KiriView::ActiveNavigationSnapshot unknownActiveNavigation()
{
    return KiriView::ActiveNavigationSnapshot { true };
}

KiriView::ActiveNavigationSnapshot normalizedActiveNavigation(
    KiriView::ActiveNavigationSnapshot snapshot)
{
    if (!snapshot.available || !snapshot.known || snapshot.currentNumber < 1 || snapshot.count < 1
        || snapshot.currentNumber > snapshot.count) {
        return snapshot.available ? unknownActiveNavigation() : unavailableActiveNavigation();
    }

    snapshot.known = true;
    snapshot.editable = true;
    return snapshot;
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

QString DocumentSessionRuntime::windowTitleFileName() const
{
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return m_imageDocument.windowTitleFileName();
    case DocumentSessionKind::Video:
        return m_videoDocument.windowTitleFileName();
    case DocumentSessionKind::Empty:
        return QString();
    }

    return QString();
}

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
    return activeNavigationSnapshot().available;
}

bool DocumentSessionRuntime::activeNavigationKnown() const
{
    return activeNavigationSnapshot().known;
}

bool DocumentSessionRuntime::activeNavigationEditable() const
{
    return activeNavigationSnapshot().editable;
}

int DocumentSessionRuntime::activeNavigationCurrentNumber() const
{
    return activeNavigationSnapshot().currentNumber;
}

int DocumentSessionRuntime::activeNavigationCount() const
{
    return activeNavigationSnapshot().count;
}

bool DocumentSessionRuntime::canOpenPreviousActiveNavigation() const
{
    return activeNavigationSnapshot().canOpenPrevious;
}

bool DocumentSessionRuntime::canOpenNextActiveNavigation() const
{
    return activeNavigationSnapshot().canOpenNext;
}

bool DocumentSessionRuntime::atKnownFirstActiveNavigation() const
{
    return activeNavigationSnapshot().atKnownFirst;
}

bool DocumentSessionRuntime::atKnownLastActiveNavigation() const
{
    return activeNavigationSnapshot().atKnownLast;
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

    loadMediaCandidates([this](std::vector<MediaNavigationCandidate> candidates) {
        finishMediaNavigation(std::move(candidates), previousMediaNavigationOpenRequest());
    });
}

void DocumentSessionRuntime::openNextMedia()
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this](std::vector<MediaNavigationCandidate> candidates) {
        finishMediaNavigation(std::move(candidates), nextMediaNavigationOpenRequest());
    });
}

void DocumentSessionRuntime::openMediaAtNumber(int mediaNumber)
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this, mediaNumber](std::vector<MediaNavigationCandidate> candidates) {
        finishMediaNavigation(
            std::move(candidates), numberedMediaNavigationOpenRequest(mediaNumber));
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
    const ActiveNavigationSnapshot snapshot = activeNavigationSnapshot();
    if (!snapshot.known || snapshot.atKnownLast) {
        return;
    }

    openActiveNavigationAtNumber(snapshot.count);
}

void DocumentSessionRuntime::openActiveNavigationAtNumber(int number)
{
    const ActiveNavigationSnapshot snapshot = activeNavigationSnapshot();
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
    loadMediaCandidates([this, mode](std::vector<MediaNavigationCandidate> candidates) mutable {
        startMediaDeletion(mode, std::move(candidates));
    });
}

void DocumentSessionRuntime::connectDocuments()
{
    QObject::connect(&m_imageDocument, &KiriImageDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::statusChanged, m_owner,
        [this]() { syncFromImageDocument(); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::windowTitleFileNameChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::WindowTitleFileName); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::documentScopeChanged, m_owner, [this]() {
        if (m_routingSource) {
            return;
        }

        refreshMediaNavigation();
        m_state.publish({ DocumentSessionChange::MediaNavigationAvailability,
            DocumentSessionChange::FileDeletionAvailability,
            DocumentSessionChange::ActiveNavigation });
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
        [this]() { m_state.publish(DocumentSessionChange::WindowTitleFileName); });
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
        m_state.publish(DocumentSessionChange::ActiveNavigation);
    }
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    m_state.setSessionErrorString(QString());
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();
    cancelMediaDeletion();
    m_state.setMediaNavigationState({}, false);

    if (sourceUrl.isEmpty()) {
        m_routingSource = true;
        leaveVideoMode();
        m_imageDocument.setSourceUrl(QUrl());
        m_routingSource = false;
        m_state.clearDirectMediaCursor();
        m_state.setSourceIdentity(QUrl());
        m_state.setDocumentKind(DocumentSessionKind::Empty);
        m_state.setMediaNavigationState({}, false);
        m_mediaPredecodeCoordinator->clear();
        return;
    }

    openMediaUrl(sourceUrl);
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    if (isDirectVideoUrl(url)) {
        m_state.setDirectVideoCursor(url);
        m_routingSource = true;
        m_imageDocument.setSourceUrl(QUrl());
        m_videoDocument.setSourceUrl(url);
        m_state.setDocumentKind(DocumentSessionKind::Video);
        m_routingSource = false;
        m_state.setSourceIdentity(url);
        refreshMediaNavigation();
        return;
    }

    if (isDirectImageUrl(url)) {
        if (m_state.documentKind() != DocumentSessionKind::Image) {
            m_state.clearDirectMediaCursor();
        }
        m_state.requestDirectImageCursor(url);
    } else {
        m_state.clearDirectMediaCursor();
    }

    m_routingSource = true;
    leaveVideoMode();
    m_imageDocument.setSourceUrl(url);
    m_state.setDocumentKind(DocumentSessionKind::Image);
    m_routingSource = false;
    syncDirectImageCursorFromDocument();
    m_state.setSourceIdentity(m_imageDocument.sourceUrl());
    refreshMediaNavigation();
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

    syncDirectImageCursorFromDocument();
    m_state.setSourceIdentity(m_imageDocument.sourceUrl());
    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
            DocumentSessionChange::MediaNavigationAvailability,
            DocumentSessionChange::ActiveNavigation });
    refreshMediaNavigation();
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

    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
            DocumentSessionChange::MediaNavigationAvailability,
            DocumentSessionChange::ActiveNavigation });
}

void DocumentSessionRuntime::refreshMediaNavigation()
{
    if (!mediaNavigationActive()) {
        m_state.setMediaNavigationState({}, false);
        if (!directImageLoadMayUseMediaScope()) {
            m_mediaPredecodeCoordinator->clear();
        }
        return;
    }

    loadMediaCandidates([this](std::vector<MediaNavigationCandidate> candidates) {
        updateMediaBoundaryState(candidates);
    });
}

void DocumentSessionRuntime::loadMediaCandidates(
    std::function<void(std::vector<MediaNavigationCandidate>)> callback)
{
    const QUrl currentUrl = activeDirectMediaCursorUrl();
    const QUrl parentUrl = activeDirectMediaScopeUrl();
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();

    if (currentUrl.isEmpty() || parentUrl.isEmpty() || !parentUrl.isValid()
        || !m_mediaCandidateProvider.directoryMedia) {
        callback({});
        return;
    }

    const DocumentSessionMediaCandidateLoad load = m_mediaCandidateLoadState.start(
        currentUrl, parentUrl, m_state.directMediaCursor().generation);
    auto sharedCallback
        = std::make_shared<std::function<void(std::vector<MediaNavigationCandidate>)>>(
            std::move(callback));
    m_mediaCandidateJob = m_mediaCandidateProvider.directoryMedia(
        m_owner, parentUrl,
        [this, load, sharedCallback](std::vector<MediaNavigationCandidate> candidates) mutable {
            finishMediaCandidateLoad(load, std::move(candidates), sharedCallback);
        },
        [this, load, sharedCallback](
            const QString &) mutable { finishMediaCandidateLoad(load, {}, sharedCallback); });
}

void DocumentSessionRuntime::finishMediaCandidateLoad(DocumentSessionMediaCandidateLoad load,
    std::vector<MediaNavigationCandidate> candidates,
    const std::shared_ptr<std::function<void(std::vector<MediaNavigationCandidate>)>> &callback)
{
    if (!m_mediaCandidateLoadState.finish(load) || !directMediaCursorMatches(load) || !callback
        || !*callback) {
        return;
    }

    (*callback)(std::move(candidates));
}

void DocumentSessionRuntime::finishMediaNavigation(
    std::vector<MediaNavigationCandidate> candidates, MediaNavigationOpenRequest request)
{
    const MediaNavigationOpenPlan plan
        = mediaNavigationOpenPlan(candidates, activeDirectMediaCursorUrl(), request);
    m_state.setMediaNavigationState(plan.boundaryState, true);
    scheduleMediaPredecode(candidates);
    if (plan.targetUrl.has_value()) {
        openMediaUrl(*plan.targetUrl);
    }
}

void DocumentSessionRuntime::updateMediaBoundaryState(
    const std::vector<MediaNavigationCandidate> &candidates)
{
    m_state.setMediaNavigationState(
        mediaNavigationBoundaryState(candidates, activeDirectMediaCursorUrl()), true);
    scheduleMediaPredecode(candidates);
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
}

void DocumentSessionRuntime::startMediaDeletion(
    FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates)
{
    const QUrl targetUrl = activeDirectMediaCursorUrl();
    if (targetUrl.isEmpty()) {
        m_state.setFileDeletionInProgress(false);
        return;
    }

    const MediaDeletionFallbackPlan fallbackPlan
        = mediaDeletionFallbackPlan(std::move(candidates), targetUrl);
    m_fileDeletionJob.cancel();
    const quint64 operationId = m_fileDeletionOperation.start();
    m_fileDeletionJob = m_fileOperationProvider(m_owner, FileDeletionRequest { targetUrl, mode },
        [this, operationId, fallbackPlan](FileDeletionResult result, const QString &errorString) {
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

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        if (m_state.documentKind() == DocumentSessionKind::Video) {
            leaveVideoMode();
        } else if (m_state.documentKind() == DocumentSessionKind::Image) {
            m_imageDocument.setSourceUrl(QUrl());
        }

        if (fallbackPlan.preferredFallbackUrl.has_value()) {
            openMediaUrl(*fallbackPlan.preferredFallbackUrl);
        } else if (fallbackPlan.fallbackUrl.has_value()) {
            openMediaUrl(*fallbackPlan.fallbackUrl);
        } else {
            m_state.setSourceIdentity(QUrl());
            m_state.setDocumentKind(DocumentSessionKind::Empty);
            m_state.setMediaNavigationState({}, false);
            m_mediaPredecodeCoordinator->clear();
        }
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
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
    return activeDirectMediaCursorUrl() == load.currentUrl
        && activeDirectMediaScopeUrl() == load.parentUrl
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

void DocumentSessionRuntime::syncDirectImageCursorFromDocument()
{
    if (m_state.documentKind() != DocumentSessionKind::Image) {
        return;
    }

    const QUrl pendingUrl = m_state.directMediaCursor().pendingUrl;
    const QUrl displayedUrl = m_imageDocument.displayedUrl();
    if (!pendingUrl.isEmpty()) {
        if (activeImageUsesMediaScope() && displayedUrl == pendingUrl) {
            m_state.confirmDirectImageCursor(displayedUrl);
            return;
        }

        if (m_imageDocument.status() == KiriImageDocument::Status::Error
            || (!m_imageDocument.sourceUrl().isEmpty()
                && m_imageDocument.sourceUrl() != pendingUrl)) {
            m_state.restoreDirectImageCursorAfterFailure();
        }
        return;
    }

    if (activeImageUsesMediaScope() && !displayedUrl.isEmpty()) {
        m_state.confirmDirectImageCursor(displayedUrl);
        return;
    }

    if (m_imageDocument.status() == KiriImageDocument::Status::Error) {
        m_state.restoreDirectImageCursorAfterFailure();
    }
}

DocumentSessionRuntime::ActiveNavigationSourceKind
DocumentSessionRuntime::activeNavigationSourceKind() const
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
                && !isDirectImageUrl(m_imageDocument.sourceUrl()))) {
            return ActiveNavigationSourceKind::ImageDocumentPages;
        }
        return ActiveNavigationSourceKind::None;
    case DocumentSessionKind::Empty:
        return ActiveNavigationSourceKind::None;
    }

    return ActiveNavigationSourceKind::None;
}

ActiveNavigationSnapshot DocumentSessionRuntime::activeNavigationSnapshot() const
{
    switch (activeNavigationSourceKind()) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return mediaActiveNavigationSnapshot();
    case ActiveNavigationSourceKind::ImageDocumentPages:
        return imageDocumentActiveNavigationSnapshot();
    case ActiveNavigationSourceKind::None:
        return unavailableActiveNavigation();
    }

    return unavailableActiveNavigation();
}

ActiveNavigationSnapshot DocumentSessionRuntime::mediaActiveNavigationSnapshot() const
{
    if (!m_state.mediaNavigationKnown()) {
        return unknownActiveNavigation();
    }

    const MediaNavigationBoundaryState &state = m_state.mediaNavigationState();
    return normalizedActiveNavigation(ActiveNavigationSnapshot {
        true,
        true,
        true,
        state.canOpenPrevious,
        state.canOpenNext,
        state.atKnownFirst,
        state.atKnownLast,
        state.currentNumber,
        state.count,
    });
}

ActiveNavigationSnapshot DocumentSessionRuntime::imageDocumentActiveNavigationSnapshot() const
{
    const int currentNumber = m_imageDocument.currentPageNumber();
    const int currentLastNumber = m_imageDocument.currentLastPageNumber();
    const int count = m_imageDocument.imageCount();
    if (currentNumber < 1 || currentLastNumber < currentNumber || count < 1
        || currentLastNumber > count) {
        return unknownActiveNavigation();
    }

    return normalizedActiveNavigation(ActiveNavigationSnapshot {
        true,
        true,
        true,
        currentNumber > 1,
        currentLastNumber < count,
        currentNumber == 1,
        currentLastNumber >= count,
        currentNumber,
        count,
    });
}
}
