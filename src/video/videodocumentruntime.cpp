// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include "video/videooutputrendercontextobserver.h"
#include "video/videozoomstate.h"

#include <QObject>
#include <algorithm>
#include <utility>

namespace {
KiriView::VideoDocumentStatus documentStatusForMediaStatus(KiriView::VideoMediaStatus status)
{
    switch (status) {
    case KiriView::VideoMediaStatus::Null:
        return KiriView::VideoDocumentStatus::Null;
    case KiriView::VideoMediaStatus::Loading:
    case KiriView::VideoMediaStatus::Stalled:
        return KiriView::VideoDocumentStatus::Loading;
    case KiriView::VideoMediaStatus::Loaded:
    case KiriView::VideoMediaStatus::Buffering:
    case KiriView::VideoMediaStatus::Buffered:
    case KiriView::VideoMediaStatus::EndOfMedia:
        return KiriView::VideoDocumentStatus::Ready;
    case KiriView::VideoMediaStatus::Invalid:
        return KiriView::VideoDocumentStatus::Error;
    }

    return KiriView::VideoDocumentStatus::Null;
}
}

namespace KiriView {
VideoDocumentRuntime::VideoDocumentRuntime(QObject *documentObject, ChangeCallback changeCallback,
    std::unique_ptr<VideoMediaBackend> mediaBackend,
    std::unique_ptr<VideoPlaybackUrlResolver> playbackUrlResolver,
    MediaBackendFactory mediaBackendFactory)
    : m_documentObject(documentObject)
    , m_state(std::move(changeCallback))
    , m_mediaBackend(std::move(mediaBackend))
    , m_mediaBackendFactory(std::move(mediaBackendFactory))
    , m_sourceLoadRuntime(std::move(playbackUrlResolver))
    , m_renderContextObserver(std::make_unique<VideoOutputRenderContextObserver>(
          documentObject, [this]() { updateZoomPercent(); }))
{
    if (!m_mediaBackendFactory) {
        m_mediaBackendFactory
            = [](QObject *parent) { return createDefaultVideoMediaBackend(parent); };
    }

    if (m_mediaBackend == nullptr) {
        return;
    }

    installMediaBackendCallbacks();
}

VideoMediaBackend *VideoDocumentRuntime::ensureMediaBackend()
{
    if (m_mediaBackend == nullptr) {
        m_mediaBackend = m_mediaBackendFactory(m_documentObject);
        installMediaBackendCallbacks();
        if (m_videoOutput != nullptr) {
            m_mediaBackend->setVideoOutput(m_videoOutput.data());
        }
    }

    return m_mediaBackend.get();
}

void VideoDocumentRuntime::installMediaBackendCallbacks()
{
    m_mediaBackend->setCallbacks(VideoMediaBackendCallbacks {
        [this]() { updateStatusFromBackend(); },
        [this]() { updateErrorFromBackend(); },
        [this]() { m_state.setDuration(m_mediaBackend->duration()); },
        [this]() { m_state.setPosition(m_mediaBackend->position()); },
        [this]() { m_state.setPlaying(m_mediaBackend->playing()); },
        [this]() { m_state.setSeekable(m_mediaBackend->seekable()); },
        [this]() {
            m_state.setHasVideo(m_mediaBackend->hasVideo());
            updateZoomPercent();
        },
        [this]() { m_state.setHasAudio(m_mediaBackend->hasAudio()); },
        {},
    });
}

VideoDocumentRuntime::~VideoDocumentRuntime()
{
    disconnectVideoOutputDestroyed();
    m_sourceLoadRuntime.shutdown();
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setVideoOutput(nullptr);
        m_mediaBackend->setSource(QUrl());
    }
}

QUrl VideoDocumentRuntime::sourceUrl() const { return m_state.sourceUrl(); }

void VideoDocumentRuntime::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_state.sourceUrl() == sourceUrl) {
        return;
    }

    m_sourceLoadRuntime.setSourceUrl(sourceUrl, m_documentObject, sourceLoadOperations());
}

VideoDocumentStatus VideoDocumentRuntime::status() const { return m_state.status(); }

QString VideoDocumentRuntime::errorString() const { return m_state.errorString(); }

QString VideoDocumentRuntime::windowTitleFileName() const { return m_state.windowTitleFileName(); }

qint64 VideoDocumentRuntime::duration() const { return m_state.duration(); }

qint64 VideoDocumentRuntime::position() const { return m_state.position(); }

void VideoDocumentRuntime::setPosition(qint64 position)
{
    if (!m_state.seekable()) {
        return;
    }

    m_state.setEnded(false);
    const qint64 upperBound = m_state.duration() > 0 ? m_state.duration() : position;
    const qint64 clamped = std::clamp(position, qint64(0), upperBound);
    ensureMediaBackend()->setPosition(clamped);
    m_state.setPosition(clamped);
}

bool VideoDocumentRuntime::playing() const { return m_state.playing(); }

bool VideoDocumentRuntime::seekable() const { return m_state.seekable(); }

bool VideoDocumentRuntime::hasVideo() const { return m_state.hasVideo(); }

bool VideoDocumentRuntime::hasAudio() const { return m_state.hasAudio(); }

bool VideoDocumentRuntime::zoomPercentKnown() const { return m_state.zoomPercentKnown(); }

int VideoDocumentRuntime::zoomPercent() const { return m_state.zoomPercent(); }

QObject *VideoDocumentRuntime::videoOutput() const { return m_videoOutput.data(); }

void VideoDocumentRuntime::setVideoOutput(QObject *videoOutput)
{
    if (m_videoOutput.data() == videoOutput
        && (m_mediaBackend == nullptr || m_mediaBackend->videoOutput() == videoOutput)) {
        return;
    }

    disconnectVideoOutputDestroyed();
    m_videoOutput = videoOutput;
    m_renderContextObserver->setVideoOutput(videoOutput);
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setVideoOutput(videoOutput);
    }
    connectVideoOutputDestroyed(videoOutput);
    publish(VideoDocumentChange::VideoOutput);
    updateZoomPercent();
}

void VideoDocumentRuntime::setVideoOutputGeometry(
    const QRectF &contentRect, const QRectF &sourceRect)
{
    if (m_videoOutputContentRect == contentRect && m_videoOutputSourceRect == sourceRect) {
        return;
    }

    m_videoOutputContentRect = contentRect;
    m_videoOutputSourceRect = sourceRect;
    updateZoomPercent();
}

void VideoDocumentRuntime::play()
{
    if (m_state.sourceUrl().isEmpty()) {
        return;
    }

    VideoMediaBackend *mediaBackend = ensureMediaBackend();
    if (m_state.ended() && m_state.seekable()) {
        mediaBackend->setPosition(0);
        m_state.setPosition(0);
    }
    m_state.setEnded(false);
    mediaBackend->play();
}

void VideoDocumentRuntime::pause()
{
    if (m_mediaBackend == nullptr) {
        return;
    }

    m_mediaBackend->pause();
}

void VideoDocumentRuntime::stop()
{
    m_state.setEnded(false);
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
    }
    m_state.setPlaying(false);
    if (m_state.seekable()) {
        if (m_mediaBackend != nullptr) {
            m_mediaBackend->setPosition(0);
        }
        m_state.setPosition(0);
    }
}

void VideoDocumentRuntime::togglePlayback()
{
    if (m_state.playing()) {
        pause();
        return;
    }

    play();
}

void VideoDocumentRuntime::seekBy(qint64 deltaMilliseconds)
{
    const qint64 nextPosition = clampedSeekPosition(
        m_state.position(), deltaMilliseconds, m_state.duration(), m_state.seekable());
    if (!m_state.seekable() || nextPosition == m_state.position()) {
        return;
    }

    m_state.setEnded(false);
    ensureMediaBackend()->setPosition(nextPosition);
    m_state.setPosition(nextPosition);
}

qint64 VideoDocumentRuntime::clampedSeekPosition(
    qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable)
{
    if (!seekable) {
        return currentPosition;
    }

    const qint64 target = currentPosition + deltaMilliseconds;
    if (duration > 0) {
        return std::clamp(target, qint64(0), duration);
    }

    return std::max<qint64>(0, target);
}

VideoSourceLoadOperations VideoDocumentRuntime::sourceLoadOperations()
{
    return VideoSourceLoadOperations {
        [this]() { clearPlaybackSource(); },
        [this]() { m_state.resetForClearedSource(); },
        [this](const QUrl &sourceUrl) { m_state.resetForSourceLoad(sourceUrl); },
        [this](
            const VideoPlaybackUrlResolution &resolution) { applyResolvedPlaybackUrl(resolution); },
        [this](const QUrl &sourceUrl, const QString &errorString) {
            publishSourceLoadFailure(sourceUrl, errorString);
        },
    };
}

void VideoDocumentRuntime::clearPlaybackSource()
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
        m_mediaBackend->setSource(QUrl());
    }
}

void VideoDocumentRuntime::applyResolvedPlaybackUrl(const VideoPlaybackUrlResolution &resolution)
{
    VideoMediaBackend *mediaBackend = ensureMediaBackend();
    mediaBackend->setSource(resolution.playbackUrl);
    updateStatusFromBackend();
    play();
}

void VideoDocumentRuntime::publishSourceLoadFailure(const QUrl &, const QString &errorString)
{
    m_state.setErrorString(errorString);
    m_state.setStatus(VideoDocumentStatus::Error);
    updateZoomPercent();
}

void VideoDocumentRuntime::connectVideoOutputDestroyed(QObject *videoOutput)
{
    if (videoOutput == nullptr) {
        return;
    }

    m_videoOutputDestroyedConnection
        = QObject::connect(videoOutput, &QObject::destroyed, m_documentObject, [this]() {
              m_videoOutput.clear();
              if (m_mediaBackend != nullptr) {
                  m_mediaBackend->setVideoOutput(nullptr);
              }
              m_renderContextObserver->setVideoOutput(nullptr);
              publish(VideoDocumentChange::VideoOutput);
              updateZoomPercent();
          });
}

void VideoDocumentRuntime::disconnectVideoOutputDestroyed()
{
    if (m_videoOutputDestroyedConnection) {
        QObject::disconnect(m_videoOutputDestroyedConnection);
        m_videoOutputDestroyedConnection = {};
    }
}

void VideoDocumentRuntime::updateStatusFromBackend()
{
    if (m_state.sourceUrl().isEmpty()) {
        m_state.setEnded(false);
        m_state.setStatus(VideoDocumentStatus::Null);
        updateZoomPercent();
        return;
    }
    if (m_sourceLoadRuntime.active()) {
        m_state.setEnded(false);
        m_state.setStatus(VideoDocumentStatus::Loading);
        updateZoomPercent();
        return;
    }
    if (m_mediaBackend == nullptr) {
        m_state.setEnded(false);
        m_state.setStatus(VideoDocumentStatus::Loading);
        updateZoomPercent();
        return;
    }

    const VideoMediaStatus mediaStatus = m_mediaBackend->mediaStatus();
    if (mediaStatus == VideoMediaStatus::Null) {
        m_state.setEnded(false);
        m_state.setStatus(VideoDocumentStatus::Loading);
        updateZoomPercent();
        return;
    }
    m_state.setEnded(mediaStatus == VideoMediaStatus::EndOfMedia);
    if (m_state.ended()) {
        m_state.setPlaying(false);
    }
    m_state.setStatus(documentStatusForMediaStatus(mediaStatus));
    updateZoomPercent();
}

void VideoDocumentRuntime::updateErrorFromBackend()
{
    if (m_mediaBackend == nullptr) {
        return;
    }

    const QString backendError = m_mediaBackend->errorString();
    if (!backendError.isEmpty()) {
        m_state.setErrorString(backendError);
        m_state.setStatus(VideoDocumentStatus::Error);
        updateZoomPercent();
    }
}

void VideoDocumentRuntime::updateZoomPercent()
{
    if (m_state.status() != VideoDocumentStatus::Ready || !m_state.hasVideo()) {
        m_state.setZoomPercent(std::nullopt);
        return;
    }

    const std::optional<qreal> devicePixelRatio = m_renderContextObserver->devicePixelRatio();
    if (!devicePixelRatio.has_value()) {
        m_state.setZoomPercent(std::nullopt);
        return;
    }

    m_state.setZoomPercent(videoZoomPercentForRects(
        m_videoOutputContentRect, m_videoOutputSourceRect, devicePixelRatio.value()));
}

void VideoDocumentRuntime::publish(VideoDocumentChange change) { m_state.publish(change); }
}
