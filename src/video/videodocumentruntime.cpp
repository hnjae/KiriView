// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include "metadata/embeddedmetadata.h"
#include "video/videodocumentstatusplan.h"

#include <QObject>
#include <utility>
#include <variant>

namespace kiriview {
VideoDocumentRuntime::VideoDocumentRuntime(QObject *documentObject, ChangeCallback changeCallback,
    std::unique_ptr<VideoMediaBackend> mediaBackend,
    std::unique_ptr<VideoPlaybackUrlResolver> playbackUrlResolver,
    MediaBackendFactory mediaBackendFactory)
    : m_documentObject(documentObject)
    , m_state(std::move(changeCallback))
    , m_mediaBackend(std::move(mediaBackend))
    , m_mediaBackendFactory(std::move(mediaBackendFactory))
    , m_sourceLoadRuntime(std::move(playbackUrlResolver))
    , m_outputRuntime(documentObject,
          VideoOutputRuntimeCallbacks {
              [this](QObject *videoOutput) {
                  if (m_mediaBackend != nullptr) {
                      m_mediaBackend->setVideoOutput(videoOutput);
                  }
              },
              [this]() { publish(VideoDocumentChange::VideoOutput); },
              [this]() { updateZoomPercent(); },
          })
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
        m_mediaBackend->setMuted(m_state.muted());
        if (m_outputRuntime.videoOutput() != nullptr) {
            m_mediaBackend->setVideoOutput(m_outputRuntime.videoOutput());
        }
    }

    return m_mediaBackend.get();
}

void VideoDocumentRuntime::installMediaBackendCallbacks()
{
    m_mediaBackend->setCallbacks(VideoMediaBackendCallbacks {
        [this]() { updateStatusFromBackend(); },
        [this]() { updateErrorFromBackend(); },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setDuration(m_mediaBackend->duration());
            }
        },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setPosition(m_mediaBackend->position());
            }
        },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setPlaying(m_mediaBackend->playing());
            }
        },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setSeekable(m_mediaBackend->seekable());
            }
        },
        [this]() {
            if (!playbackCallbacksAccepted()) {
                return;
            }
            m_state.setHasVideo(m_mediaBackend->hasVideo());
            updateZoomPercent();
        },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setHasAudio(m_mediaBackend->hasAudio());
            }
        },
        [this]() {
            if (playbackCallbacksAccepted()) {
                m_state.setVideoSize(m_mediaBackend->videoSize());
            }
        },
        [this]() { m_state.setMuted(m_mediaBackend->muted()); },
        {},
    });
}

VideoDocumentRuntime::~VideoDocumentRuntime()
{
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

const std::optional<VideoSourceLoadFailure> &VideoDocumentRuntime::sourceLoadFailure() const
{
    return m_state.sourceLoadFailure();
}

const std::optional<VideoBackendFailure> &VideoDocumentRuntime::backendFailure() const
{
    return m_state.backendFailure();
}

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

bool VideoDocumentRuntime::muted() const { return m_state.muted(); }

void VideoDocumentRuntime::setMuted(bool muted)
{
    m_state.setMuted(muted);
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setMuted(m_state.muted());
    }
}

QObject *VideoDocumentRuntime::videoOutput() const { return m_outputRuntime.videoOutput(); }

const EmbeddedMetadata &VideoDocumentRuntime::embeddedMetadata() const
{
    return m_state.embeddedMetadata();
}

void VideoDocumentRuntime::setVideoOutput(QObject *videoOutput)
{
    m_outputRuntime.setVideoOutput(videoOutput);
}

void VideoDocumentRuntime::setVideoOutputGeometry(
    const QRectF &contentRect, const QRectF &sourceRect)
{
    m_outputRuntime.setVideoOutputGeometry(contentRect, sourceRect);
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

void VideoDocumentRuntime::toggleMuted() { setMuted(!m_state.muted()); }

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
        m_state.mediaEnded(),
        m_state.seekable(),
        m_state.position(),
        m_state.duration(),
    };
}

void VideoDocumentRuntime::executePlaybackControlPlan(const VideoPlaybackControlPlan &plan)
{
    for (const VideoPlaybackBackendOperation &operation : plan.backendOperations) {
        executePlaybackBackendOperation(operation);
    }

    applyPlaybackStateDelta(plan.stateDelta);
}

void VideoDocumentRuntime::executePlaybackBackendOperation(
    const VideoPlaybackBackendOperation &operation)
{
    std::visit(
        [this](const auto &payload) { executePlaybackBackendOperation(payload); }, operation);
}

void VideoDocumentRuntime::executePlaybackBackendOperation(
    const EnsureVideoPlaybackBackendOperation &)
{
    ensureMediaBackend();
}

void VideoDocumentRuntime::executePlaybackBackendOperation(const PlayVideoPlaybackOperation &)
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->play();
    }
}

void VideoDocumentRuntime::executePlaybackBackendOperation(const PauseVideoPlaybackOperation &)
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->pause();
    }
}

void VideoDocumentRuntime::executePlaybackBackendOperation(const StopVideoPlaybackOperation &)
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
    }
}

void VideoDocumentRuntime::executePlaybackBackendOperation(
    const SetVideoPlaybackPositionOperation &operation)
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setPosition(operation.position);
    }
}

void VideoDocumentRuntime::applyPlaybackStateDelta(const VideoPlaybackStateDelta &delta)
{
    if (delta.mediaEnded.has_value()) {
        m_state.setMediaEnded(delta.mediaEnded.value());
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
    std::visit([this](const auto &payload) { executeSourceLoadOperation(payload); }, operation);
}

void VideoDocumentRuntime::executeSourceLoadOperation(const ClearVideoPlaybackSourceOperation &)
{
    invalidatePlaybackCallbacks();
    clearPlaybackSource();
}

void VideoDocumentRuntime::executeSourceLoadOperation(const ResetClearedVideoSourceOperation &)
{
    m_state.resetForClearedSource();
}

void VideoDocumentRuntime::executeSourceLoadOperation(
    const ResetVideoSourceLoadOperation &operation)
{
    invalidatePlaybackCallbacks();
    m_state.resetForSourceLoad(operation.sourceUrl);
}

void VideoDocumentRuntime::executeSourceLoadOperation(
    const ApplyVideoPlaybackUrlOperation &operation)
{
    applyResolvedPlaybackUrl(operation.playbackUrl);
}

void VideoDocumentRuntime::executeSourceLoadOperation(
    const PublishVideoSourceLoadFailureOperation &operation)
{
    publishSourceLoadFailure(operation.failure);
}

void VideoDocumentRuntime::applyResolvedPlaybackUrl(const QUrl &playbackUrl)
{
    VideoMediaBackend *mediaBackend = ensureMediaBackend();
    acceptPlaybackCallbacks();
    mediaBackend->setSource(playbackUrl);
    m_state.setEmbeddedMetadata(playbackUrl.isLocalFile()
            ? parsePathEmbeddedMetadata(playbackUrl.toLocalFile())
            : EmbeddedMetadata {});
    m_state.setVideoSize(mediaBackend->videoSize());
    updateStatusFromBackend();
    play();
}

void VideoDocumentRuntime::publishSourceLoadFailure(const VideoSourceLoadFailure &failure)
{
    invalidatePlaybackCallbacks();
    m_state.setEmbeddedMetadata({});
    m_state.setSourceLoadFailure(failure);
    updateZoomPercent();
}

void VideoDocumentRuntime::invalidatePlaybackCallbacks()
{
    ++m_playbackGeneration;
    m_acceptedPlaybackGeneration = 0;
}

void VideoDocumentRuntime::acceptPlaybackCallbacks()
{
    if (m_playbackGeneration == 0) {
        m_playbackGeneration = 1;
    }
    m_acceptedPlaybackGeneration = m_playbackGeneration;
}

bool VideoDocumentRuntime::playbackCallbacksAccepted() const
{
    return m_playbackGeneration == 0
        || (m_acceptedPlaybackGeneration != 0
            && m_acceptedPlaybackGeneration == m_playbackGeneration);
}

void VideoDocumentRuntime::updateStatusFromBackend()
{
    if (!playbackCallbacksAccepted()) {
        return;
    }

    const VideoDocumentStatusPlan plan = videoDocumentStatusPlan(VideoDocumentStatusSnapshot {
        m_state.sourceUrl().isEmpty(),
        m_sourceLoadRuntime.active(),
        m_mediaBackend != nullptr,
        m_mediaBackend != nullptr ? m_mediaBackend->mediaStatus() : VideoMediaStatus::Null,
    });
    m_state.setMediaEnded(plan.mediaEnded);
    if (plan.clearPlaying) {
        m_state.setPlaying(false);
    }
    m_state.setStatusAndError(
        plan.status, plan.status == VideoDocumentStatus::Error ? m_state.errorString() : QString());
    updateZoomPercent();
}

void VideoDocumentRuntime::updateErrorFromBackend()
{
    if (m_mediaBackend == nullptr || !playbackCallbacksAccepted()) {
        return;
    }

    const QString backendError = m_mediaBackend->errorString();
    if (!backendError.isEmpty()) {
        m_state.setBackendFailure(VideoBackendFailure {
            m_state.sourceUrl(),
            VideoBackendFailureKind::Playback,
            backendError,
            backendError,
            VideoBackendFailureSeverity::Error,
            false,
        });
        updateZoomPercent();
    }
}

void VideoDocumentRuntime::updateZoomPercent()
{
    if (m_state.status() != VideoDocumentStatus::Ready || !m_state.hasVideo()) {
        m_state.setZoomPercent(std::nullopt);
        return;
    }

    const std::optional<int> zoomPercent = m_outputRuntime.zoomPercent();
    if (!zoomPercent.has_value()) {
        m_state.setZoomPercent(std::nullopt);
        return;
    }

    m_state.setZoomPercent(zoomPercent.value());
}

void VideoDocumentRuntime::publish(VideoDocumentChange change) { m_state.publish(change); }
}
