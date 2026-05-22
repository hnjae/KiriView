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
#include <algorithm>
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

bool DocumentSessionRuntime::mediaNavigationActive() const
{
    return m_state.documentKind() == DocumentSessionKind::Video
        || (m_state.documentKind() == DocumentSessionKind::Image && activeImageUsesMediaScope());
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
        updateMediaBoundaryState(candidates);
        const std::optional<QUrl> target = adjacentMediaNavigationUrl(
            candidates, currentMediaUrl(), NavigationDirection::Previous);
        if (target.has_value()) {
            openMediaUrl(*target);
        }
    });
}

void DocumentSessionRuntime::openNextMedia()
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this](std::vector<MediaNavigationCandidate> candidates) {
        updateMediaBoundaryState(candidates);
        const std::optional<QUrl> target
            = adjacentMediaNavigationUrl(candidates, currentMediaUrl(), NavigationDirection::Next);
        if (target.has_value()) {
            openMediaUrl(*target);
        }
    });
}

void DocumentSessionRuntime::openMediaAtNumber(int mediaNumber)
{
    if (!mediaNavigationActive()) {
        return;
    }

    loadMediaCandidates([this, mediaNumber](std::vector<MediaNavigationCandidate> candidates) {
        updateMediaBoundaryState(candidates);
        if (candidates.empty()) {
            return;
        }

        const int clampedNumber = std::clamp(mediaNumber, 1, static_cast<int>(candidates.size()));
        openMediaUrl(candidates.at(static_cast<std::size_t>(clampedNumber - 1)).url);
    });
}

void DocumentSessionRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_state.documentKind() == DocumentSessionKind::Image && !activeImageUsesMediaScope()) {
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
            DocumentSessionChange::FileDeletionAvailability });
    });
    QObject::connect(&m_imageDocument, &KiriImageDocument::fileDeletionInProgressChanged, m_owner,
        [this]() { syncImageDocumentFileDeletionProgress(); });

    QObject::connect(&m_videoDocument, &KiriVideoDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::statusChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::windowTitleFileNameChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::WindowTitleFileName); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::errorStringChanged, m_owner,
        [this]() { m_state.publish(DocumentSessionChange::ErrorString); });
}

void DocumentSessionRuntime::syncImageDocumentFileDeletionProgress()
{
    if (m_state.documentKind() != DocumentSessionKind::Image || activeImageUsesMediaScope()) {
        return;
    }

    m_state.setFileDeletionInProgress(m_imageDocument.fileDeletionInProgress());
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    m_state.setSessionErrorString(QString());
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();
    cancelMediaDeletion();

    if (sourceUrl.isEmpty()) {
        m_routingSource = true;
        leaveVideoMode();
        m_imageDocument.setSourceUrl(QUrl());
        m_routingSource = false;
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
        m_routingSource = true;
        m_imageDocument.setSourceUrl(QUrl());
        m_state.setDocumentKind(DocumentSessionKind::Video);
        m_videoDocument.setSourceUrl(url);
        m_routingSource = false;
        m_state.setSourceIdentity(url);
        refreshMediaNavigation();
        return;
    }

    m_routingSource = true;
    leaveVideoMode();
    m_state.setDocumentKind(DocumentSessionKind::Image);
    m_imageDocument.setSourceUrl(url);
    m_routingSource = false;
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

    m_state.setSourceIdentity(m_imageDocument.sourceUrl());
    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
            DocumentSessionChange::MediaNavigationAvailability });
    refreshMediaNavigation();
}

void DocumentSessionRuntime::syncFromVideoDocument()
{
    if (m_routingSource || m_state.documentKind() != DocumentSessionKind::Video) {
        return;
    }

    if (m_videoDocument.sourceUrl().isEmpty()) {
        m_state.setSourceIdentity(QUrl());
        m_state.setDocumentKind(DocumentSessionKind::Empty);
        m_state.setMediaNavigationState({}, false);
    } else {
        m_state.setSourceIdentity(m_videoDocument.sourceUrl());
        refreshMediaNavigation();
    }

    m_state.publish(
        { DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
            DocumentSessionChange::MediaNavigationAvailability });
}

void DocumentSessionRuntime::refreshMediaNavigation()
{
    if (!mediaNavigationActive()) {
        m_state.setMediaNavigationState({}, false);
        if (!pendingDirectImageLoadMayUseMediaScope()) {
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
    const QUrl currentUrl = currentMediaUrl();
    const QUrl parentUrl = mediaNavigationParentUrl(currentUrl);
    m_mediaCandidateJob.cancel();
    m_mediaCandidateLoadState.cancel();

    if (currentUrl.isEmpty() || parentUrl.isEmpty() || !parentUrl.isValid()
        || !m_mediaCandidateProvider.directoryMedia) {
        callback({});
        return;
    }

    const DocumentSessionMediaCandidateLoad load = m_mediaCandidateLoadState.start(currentUrl);
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
    if (!m_mediaCandidateLoadState.finish(load) || !callback || !*callback) {
        return;
    }

    (*callback)(std::move(candidates));
}

void DocumentSessionRuntime::updateMediaBoundaryState(
    const std::vector<MediaNavigationCandidate> &candidates)
{
    m_state.setMediaNavigationState(
        mediaNavigationBoundaryState(candidates, currentMediaUrl()), true);
    scheduleMediaPredecode(candidates);
}

void DocumentSessionRuntime::scheduleMediaPredecode(
    const std::vector<MediaNavigationCandidate> &candidates)
{
    if (!mediaNavigationActive() || m_mediaPredecodeCoordinator == nullptr) {
        return;
    }

    const QUrl currentUrl = currentMediaUrl();
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
    const QUrl targetUrl = currentMediaUrl();
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

QUrl DocumentSessionRuntime::currentMediaUrl() const
{
    switch (m_state.documentKind()) {
    case DocumentSessionKind::Image:
        return activeImageUsesMediaScope() ? m_imageDocument.displayedUrl() : QUrl();
    case DocumentSessionKind::Video:
        return m_videoDocument.sourceUrl();
    case DocumentSessionKind::Empty:
        return QUrl();
    }

    return QUrl();
}

bool DocumentSessionRuntime::activeImageUsesMediaScope() const
{
    return m_imageDocument.ordinaryDirectMediaScopeActive();
}

bool DocumentSessionRuntime::pendingDirectImageLoadMayUseMediaScope() const
{
    return m_state.documentKind() == DocumentSessionKind::Image
        && !m_imageDocument.sourceUrl().isEmpty() && isDirectImageUrl(m_imageDocument.sourceUrl());
}
}
