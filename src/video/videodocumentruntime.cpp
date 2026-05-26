// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include "video/videodocumentstatusplan.h"
#include "video/videooutputrendercontextobserver.h"
#include "video/videozoomstate.h"

#include <QObject>
#include <utility>

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
        [this]() { m_state.setVideoSize(m_mediaBackend->videoSize()); },
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

    m_sourceLoadRuntime.setSourceUrl(sourceUrl, m_documentObject,
        [this](VideoSourceLoadPlan plan) { executeSourceLoadPlan(plan); });
}

VideoDocumentStatus VideoDocumentRuntime::status() const { return m_state.status(); }

QString VideoDocumentRuntime::errorString() const { return m_state.errorString(); }

QString VideoDocumentRuntime::windowTitleFileName() const { return m_state.windowTitleFileName(); }

qint64 VideoDocumentRuntime::duration() const { return m_state.duration(); }

qint64 VideoDocumentRuntime::position() const { return m_state.position(); }

void VideoDocumentRuntime::setPosition(qint64 position)
{
    executePlaybackControlPlan(videoPlaybackSetPositionPlan(playbackControlSnapshot(), position));
}

bool VideoDocumentRuntime::playing() const { return m_state.playing(); }

bool VideoDocumentRuntime::seekable() const { return m_state.seekable(); }

bool VideoDocumentRuntime::hasVideo() const { return m_state.hasVideo(); }

bool VideoDocumentRuntime::hasAudio() const { return m_state.hasAudio(); }

QSize VideoDocumentRuntime::videoSize() const { return m_state.videoSize(); }

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
    executePlaybackControlPlan(videoPlaybackPlayPlan(playbackControlSnapshot()));
}

void VideoDocumentRuntime::pause()
{
    executePlaybackControlPlan(videoPlaybackPausePlan(playbackControlSnapshot()));
}

void VideoDocumentRuntime::stop()
{
    executePlaybackControlPlan(videoPlaybackStopPlan(playbackControlSnapshot()));
}

void VideoDocumentRuntime::togglePlayback()
{
    executePlaybackControlPlan(videoPlaybackTogglePlan(playbackControlSnapshot()));
}

void VideoDocumentRuntime::seekBy(qint64 deltaMilliseconds)
{
    executePlaybackControlPlan(
        videoPlaybackSeekByPlan(playbackControlSnapshot(), deltaMilliseconds));
}

qint64 VideoDocumentRuntime::clampedSeekPosition(
    qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable)
{
    return videoPlaybackClampedSeekPosition(currentPosition, deltaMilliseconds, duration, seekable);
}

VideoPlaybackControlSnapshot VideoDocumentRuntime::playbackControlSnapshot() const
{
    return VideoPlaybackControlSnapshot {
        m_state.sourceUrl().isEmpty(),
        m_mediaBackend != nullptr,
        m_state.playing(),
        m_state.ended(),
        m_state.seekable(),
        m_state.position(),
        m_state.duration(),
    };
}

void VideoDocumentRuntime::executePlaybackControlPlan(const VideoPlaybackControlPlan &plan)
{
    VideoMediaBackend *mediaBackend = m_mediaBackend.get();
    for (const VideoPlaybackBackendOperation &operation : plan.backendOperations) {
        switch (operation.kind) {
        case VideoPlaybackBackendOperationKind::EnsureBackend:
            mediaBackend = ensureMediaBackend();
            break;
        case VideoPlaybackBackendOperationKind::Play:
            if (mediaBackend != nullptr) {
                mediaBackend->play();
            }
            break;
        case VideoPlaybackBackendOperationKind::Pause:
            if (mediaBackend != nullptr) {
                mediaBackend->pause();
            }
            break;
        case VideoPlaybackBackendOperationKind::Stop:
            if (mediaBackend != nullptr) {
                mediaBackend->stop();
            }
            break;
        case VideoPlaybackBackendOperationKind::SetPosition:
            if (mediaBackend != nullptr) {
                mediaBackend->setPosition(operation.position);
            }
            break;
        }
    }

    applyPlaybackStateDelta(plan.stateDelta);
}

void VideoDocumentRuntime::applyPlaybackStateDelta(const VideoPlaybackStateDelta &delta)
{
    if (delta.ended.has_value()) {
        m_state.setEnded(delta.ended.value());
    }
    if (delta.playing.has_value()) {
        m_state.setPlaying(delta.playing.value());
    }
    if (delta.position.has_value()) {
        m_state.setPosition(delta.position.value());
    }
}

void VideoDocumentRuntime::clearPlaybackSource()
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
        m_mediaBackend->setSource(QUrl());
    }
}

void VideoDocumentRuntime::executeSourceLoadPlan(const VideoSourceLoadPlan &plan)
{
    for (const VideoSourceLoadOperation &operation : plan) {
        executeSourceLoadOperation(operation);
    }
}

void VideoDocumentRuntime::executeSourceLoadOperation(const VideoSourceLoadOperation &operation)
{
    switch (operation.kind) {
    case VideoSourceLoadOperationKind::ClearPlaybackSource:
        clearPlaybackSource();
        break;
    case VideoSourceLoadOperationKind::ResetClearedSource:
        m_state.resetForClearedSource();
        break;
    case VideoSourceLoadOperationKind::ResetSourceLoad:
        m_state.resetForSourceLoad(operation.sourceUrl);
        break;
    case VideoSourceLoadOperationKind::ApplyPlaybackUrl:
        applyResolvedPlaybackUrl(operation.playbackUrl);
        break;
    case VideoSourceLoadOperationKind::PublishSourceLoadFailure:
        publishSourceLoadFailure(operation.sourceUrl, operation.errorString);
        break;
    }
}

void VideoDocumentRuntime::applyResolvedPlaybackUrl(const QUrl &playbackUrl)
{
    VideoMediaBackend *mediaBackend = ensureMediaBackend();
    mediaBackend->setSource(playbackUrl);
    m_state.setVideoSize(mediaBackend->videoSize());
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
    const VideoDocumentStatusPlan plan = videoDocumentStatusPlan(VideoDocumentStatusSnapshot {
        m_state.sourceUrl().isEmpty(),
        m_sourceLoadRuntime.active(),
        m_mediaBackend != nullptr,
        m_mediaBackend != nullptr ? m_mediaBackend->mediaStatus() : VideoMediaStatus::Null,
    });
    m_state.setEnded(plan.ended);
    if (plan.stopPublicPlayback) {
        m_state.setPlaying(false);
    }
    m_state.setStatus(plan.status);
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
