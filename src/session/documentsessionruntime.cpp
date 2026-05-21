// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionruntime.h"

#include "async/imagecallback.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "localization/imageerrortext.h"
#include "navigation/mediaformatregistry.h"

#include <QObject>
#include <QString>
#include <algorithm>
#include <memory>
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
    , m_changeCallback(std::move(changeCallback))
    , m_mediaCandidateProvider(mediaNavigationCandidateProviderWithDefault(
          std::move(dependencies.mediaCandidateProvider)))
    , m_fileOperationProvider(
          fileOperationProviderWithDefault(std::move(dependencies.fileOperationProvider)))
{
    connectDocuments();
}

DocumentSessionRuntime::~DocumentSessionRuntime()
{
    m_mediaCandidateJob.cancel();
    m_fileDeletionJob.cancel();
}

QUrl DocumentSessionRuntime::sourceUrl() const { return m_sourceUrl; }

void DocumentSessionRuntime::setSourceUrl(const QUrl &sourceUrl) { routeSourceUrl(sourceUrl); }

DocumentSessionKind DocumentSessionRuntime::documentKind() const { return m_documentKind; }

QString DocumentSessionRuntime::errorString() const
{
    if (!m_sessionErrorString.isEmpty()) {
        return m_sessionErrorString;
    }

    switch (m_documentKind) {
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
    switch (m_documentKind) {
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

    switch (m_documentKind) {
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
    return m_fileDeletionInProgress
        || (m_documentKind == DocumentSessionKind::Image
            && m_imageDocument.fileDeletionInProgress());
}

bool DocumentSessionRuntime::mediaNavigationActive() const
{
    return m_documentKind == DocumentSessionKind::Video
        || (m_documentKind == DocumentSessionKind::Image && activeImageUsesMediaScope());
}

bool DocumentSessionRuntime::canOpenPreviousMedia() const
{
    return mediaNavigationActive() && m_mediaNavigationKnown
        && m_mediaNavigationState.canOpenPrevious;
}

bool DocumentSessionRuntime::canOpenNextMedia() const
{
    return mediaNavigationActive() && m_mediaNavigationKnown && m_mediaNavigationState.canOpenNext;
}

bool DocumentSessionRuntime::atKnownFirstMedia() const
{
    return mediaNavigationActive() && m_mediaNavigationKnown && m_mediaNavigationState.atKnownFirst;
}

bool DocumentSessionRuntime::atKnownLastMedia() const
{
    return mediaNavigationActive() && m_mediaNavigationKnown && m_mediaNavigationState.atKnownLast;
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

void DocumentSessionRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_documentKind == DocumentSessionKind::Image && !activeImageUsesMediaScope()) {
        m_imageDocument.deleteDisplayedFile(mode == FileDeletionMode::MoveToTrash
                ? KiriImageDocument::DeletionMode::MoveToTrash
                : KiriImageDocument::DeletionMode::DeletePermanently);
        publish({ DocumentSessionChange::FileDeletionInProgress,
            DocumentSessionChange::FileDeletionAvailability });
        return;
    }

    if (!displayedFileDeletionAvailable() || !mediaNavigationActive()) {
        return;
    }

    setFileDeletionInProgress(true);
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
        [this]() { publish(DocumentSessionChange::WindowTitleFileName); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::errorStringChanged, m_owner,
        [this]() { publish(DocumentSessionChange::ErrorString); });
    QObject::connect(&m_imageDocument, &KiriImageDocument::documentScopeChanged, m_owner, [this]() {
        refreshMediaNavigation();
        publish({ DocumentSessionChange::MediaNavigationAvailability,
            DocumentSessionChange::FileDeletionAvailability });
    });
    QObject::connect(
        &m_imageDocument, &KiriImageDocument::fileDeletionInProgressChanged, m_owner, [this]() {
            publish({ DocumentSessionChange::FileDeletionInProgress,
                DocumentSessionChange::FileDeletionAvailability });
        });

    QObject::connect(&m_videoDocument, &KiriVideoDocument::sourceUrlChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::statusChanged, m_owner,
        [this]() { syncFromVideoDocument(); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::windowTitleFileNameChanged, m_owner,
        [this]() { publish(DocumentSessionChange::WindowTitleFileName); });
    QObject::connect(&m_videoDocument, &KiriVideoDocument::errorStringChanged, m_owner,
        [this]() { publish(DocumentSessionChange::ErrorString); });
}

void DocumentSessionRuntime::routeSourceUrl(const QUrl &sourceUrl)
{
    setSessionErrorString(QString());
    m_mediaCandidateJob.cancel();

    if (sourceUrl.isEmpty()) {
        leaveVideoMode();
        m_imageDocument.setSourceUrl(QUrl());
        setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        setMediaNavigationState({}, false);
        return;
    }

    openMediaUrl(sourceUrl);
}

void DocumentSessionRuntime::openMediaUrl(const QUrl &url)
{
    if (isDirectVideoUrl(url)) {
        m_routingSource = true;
        m_imageDocument.setSourceUrl(QUrl());
        m_routingSource = false;
        setDocumentKind(DocumentSessionKind::Video);
        m_videoDocument.setSourceUrl(url);
        setSourceIdentity(url);
        refreshMediaNavigation();
        return;
    }

    leaveVideoMode();
    setDocumentKind(DocumentSessionKind::Image);
    m_imageDocument.setSourceUrl(url);
    setSourceIdentity(m_imageDocument.sourceUrl());
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
    if (m_routingSource || m_documentKind != DocumentSessionKind::Image) {
        return;
    }

    setSourceIdentity(m_imageDocument.sourceUrl());
    publish({ DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
        DocumentSessionChange::MediaNavigationAvailability });
    refreshMediaNavigation();
}

void DocumentSessionRuntime::syncFromVideoDocument()
{
    if (m_documentKind != DocumentSessionKind::Video) {
        return;
    }

    if (m_videoDocument.sourceUrl().isEmpty()) {
        setSourceIdentity(QUrl());
        setDocumentKind(DocumentSessionKind::Empty);
        setMediaNavigationState({}, false);
    } else {
        setSourceIdentity(m_videoDocument.sourceUrl());
        refreshMediaNavigation();
    }

    publish({ DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
        DocumentSessionChange::MediaNavigationAvailability });
}

void DocumentSessionRuntime::refreshMediaNavigation()
{
    if (!mediaNavigationActive()) {
        setMediaNavigationState({}, false);
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
    if (currentUrl.isEmpty() || parentUrl.isEmpty() || !parentUrl.isValid()
        || !m_mediaCandidateProvider.directoryMedia) {
        callback({});
        return;
    }

    m_mediaCandidateJob.cancel();
    auto sharedCallback
        = std::make_shared<std::function<void(std::vector<MediaNavigationCandidate>)>>(
            std::move(callback));
    m_mediaCandidateJob = m_mediaCandidateProvider.directoryMedia(
        m_owner, parentUrl,
        [sharedCallback](std::vector<MediaNavigationCandidate> candidates) mutable {
            if (*sharedCallback) {
                (*sharedCallback)(std::move(candidates));
            }
        },
        [sharedCallback](const QString &) mutable {
            if (*sharedCallback) {
                (*sharedCallback)({});
            }
        });
}

void DocumentSessionRuntime::updateMediaBoundaryState(
    const std::vector<MediaNavigationCandidate> &candidates)
{
    setMediaNavigationState(mediaNavigationBoundaryState(candidates, currentMediaUrl()), true);
}

void DocumentSessionRuntime::startMediaDeletion(
    FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates)
{
    const QUrl targetUrl = currentMediaUrl();
    if (targetUrl.isEmpty()) {
        setFileDeletionInProgress(false);
        return;
    }

    const MediaDeletionFallbackPlan fallbackPlan
        = mediaDeletionFallbackPlan(std::move(candidates), targetUrl);
    m_fileDeletionJob.cancel();
    m_fileDeletionJob = m_fileOperationProvider(m_owner, FileDeletionRequest { targetUrl, mode },
        [this, fallbackPlan](FileDeletionResult result, const QString &errorString) {
            finishMediaDeletion(fallbackPlan, result, errorString);
        });
}

void DocumentSessionRuntime::finishMediaDeletion(const MediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result, const QString &errorString)
{
    setFileDeletionInProgress(false);

    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback:
        if (m_documentKind == DocumentSessionKind::Video) {
            leaveVideoMode();
        } else if (m_documentKind == DocumentSessionKind::Image) {
            m_imageDocument.setSourceUrl(QUrl());
        }

        if (fallbackPlan.preferredFallbackUrl.has_value()) {
            openMediaUrl(*fallbackPlan.preferredFallbackUrl);
        } else if (fallbackPlan.fallbackUrl.has_value()) {
            openMediaUrl(*fallbackPlan.fallbackUrl);
        } else {
            setSourceIdentity(QUrl());
            setDocumentKind(DocumentSessionKind::Empty);
            setMediaNavigationState({}, false);
        }
        return;
    case FileDeletionCompletionAction::Ignore:
        return;
    case FileDeletionCompletionAction::ReportFailure:
        setSessionErrorString(
            errorString.isEmpty() ? genericFileDeletionErrorMessage() : errorString);
        return;
    }
}

QUrl DocumentSessionRuntime::currentMediaUrl() const
{
    switch (m_documentKind) {
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

void DocumentSessionRuntime::setSourceIdentity(const QUrl &url)
{
    if (m_sourceUrl == url) {
        return;
    }

    m_sourceUrl = url;
    publish(DocumentSessionChange::SourceUrl);
}

void DocumentSessionRuntime::setDocumentKind(DocumentSessionKind kind)
{
    if (m_documentKind == kind) {
        return;
    }

    m_documentKind = kind;
    publish({ DocumentSessionChange::DocumentKind, DocumentSessionChange::WindowTitleFileName,
        DocumentSessionChange::ErrorString, DocumentSessionChange::FileDeletionAvailability,
        DocumentSessionChange::MediaNavigationAvailability });
}

void DocumentSessionRuntime::setFileDeletionInProgress(bool inProgress)
{
    if (m_fileDeletionInProgress == inProgress) {
        return;
    }

    m_fileDeletionInProgress = inProgress;
    publish({ DocumentSessionChange::FileDeletionInProgress,
        DocumentSessionChange::FileDeletionAvailability });
}

void DocumentSessionRuntime::setMediaNavigationState(MediaNavigationBoundaryState state, bool known)
{
    if (m_mediaNavigationKnown == known
        && m_mediaNavigationState.canOpenPrevious == state.canOpenPrevious
        && m_mediaNavigationState.canOpenNext == state.canOpenNext
        && m_mediaNavigationState.atKnownFirst == state.atKnownFirst
        && m_mediaNavigationState.atKnownLast == state.atKnownLast) {
        return;
    }

    m_mediaNavigationKnown = known;
    m_mediaNavigationState = state;
    publish(DocumentSessionChange::MediaNavigationAvailability);
}

void DocumentSessionRuntime::setSessionErrorString(const QString &errorString)
{
    if (m_sessionErrorString == errorString) {
        return;
    }

    m_sessionErrorString = errorString;
    publish(DocumentSessionChange::ErrorString);
}

void DocumentSessionRuntime::publish(DocumentSessionChange change)
{
    publish(std::vector<DocumentSessionChange> { change });
}

void DocumentSessionRuntime::publish(std::vector<DocumentSessionChange> changes)
{
    if (changes.empty()) {
        return;
    }

    std::vector<DocumentSessionChange> uniqueChanges;
    for (DocumentSessionChange change : changes) {
        if (std::find(uniqueChanges.cbegin(), uniqueChanges.cend(), change)
            == uniqueChanges.cend()) {
            uniqueChanges.push_back(change);
        }
    }
    invokeIfSet(m_changeCallback, uniqueChanges);
}
}
