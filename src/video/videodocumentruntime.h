// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTRUNTIME_H
#define KIRIVIEW_VIDEODOCUMENTRUNTIME_H

#include "metadata/embeddedmetadata.h"
#include "video/videodocumentstate.h"
#include "video/videomediabackend.h"
#include "video/videooutputruntime.h"
#include "video/videoplaybackcontrolplan.h"
#include "video/videoplaybacksource.h"
#include "video/videosourceloadplan.h"
#include "video/videosourceloadruntime.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace kiriview {
class VideoDocumentRuntime final
{
public:
    using ChangeCallback = std::function<void(const std::vector<VideoDocumentChange>&)>;
    using MediaBackendFactory = std::function<std::unique_ptr<VideoMediaBackend>(QObject*)>;

    explicit VideoDocumentRuntime(QObject* documentObject, ChangeCallback changeCallback = {},
        std::unique_ptr<VideoMediaBackend> mediaBackend = {},
        std::unique_ptr<VideoPlaybackUrlResolver> playbackUrlResolver = {},
        MediaBackendFactory mediaBackendFactory = {});
    ~VideoDocumentRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl& sourceUrl);
    void setSourceDevice(const QUrl& sourceUrl, VideoPlaybackSourceDevice sourceDevice);
    VideoDocumentStatus status() const;
    QString errorString() const;
    const std::optional<VideoSourceLoadFailure>& sourceLoadFailure() const;
    const std::optional<VideoBackendFailure>& backendFailure() const;
    QString windowTitleFileName() const;
    qint64 duration() const;
    qint64 position() const;
    void setPosition(qint64 position);
    bool playing() const;
    bool seekable() const;
    bool hasVideo() const;
    bool hasAudio() const;
    QSize videoSize() const;
    bool zoomPercentKnown() const;
    int zoomPercent() const;
    bool muted() const;
    void setMuted(bool muted);
    QObject* videoOutput() const;
    const EmbeddedMetadata& embeddedMetadata() const;
    void setVideoOutput(QObject* videoOutput);
    void setVideoOutputGeometry(const QRectF& contentRect, const QRectF& sourceRect);

    void play();
    void pause();
    void stop();
    void togglePlayback();
    void toggleMuted();
    void seekBy(qint64 deltaMilliseconds);

    static qint64 clampedSeekPosition(
        qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable);

private:
    VideoPlaybackControlSnapshot playbackControlSnapshot() const;
    void executePlaybackControlPlan(const VideoPlaybackControlPlan& plan);
    void executePlaybackBackendOperation(VideoPlaybackBackendOperation operation);
    void executePlaybackBackendOperation(EnsureVideoPlaybackBackendOperation operation);
    void executePlaybackBackendOperation(PlayVideoPlaybackOperation operation);
    void executePlaybackBackendOperation(PauseVideoPlaybackOperation operation);
    void executePlaybackBackendOperation(StopVideoPlaybackOperation operation);
    void executePlaybackBackendOperation(SetVideoPlaybackPositionOperation operation);
    void applyPlaybackStateDelta(const VideoPlaybackStateDelta& delta);
    VideoMediaBackend* ensureMediaBackend();
    void installMediaBackendCallbacks();
    void executeSourceLoadPlan(const VideoSourceLoadPlan& plan);
    void executeSourceLoadOperation(const VideoSourceLoadOperation& operation);
    void executeSourceLoadOperation(ClearVideoPlaybackSourceOperation operation);
    void executeSourceLoadOperation(ResetClearedVideoSourceOperation operation);
    void executeSourceLoadOperation(const ResetVideoSourceLoadOperation& operation);
    void executeSourceLoadOperation(const ApplyVideoPlaybackUrlOperation& operation);
    void executeSourceLoadOperation(const PublishVideoSourceLoadFailureOperation& operation);
    void clearPlaybackSource();
    void applyResolvedPlaybackUrl(const QUrl& playbackUrl);
    void applyPlaybackSourceDevice(VideoPlaybackSourceDevice sourceDevice, const QUrl& sourceUrl);
    void publishSourceLoadFailure(const VideoSourceLoadFailure& failure);
    void invalidatePlaybackCallbacks();
    void acceptPlaybackCallbacks();
    bool playbackCallbacksAccepted() const;
    void updateStatusFromBackend();
    void updateErrorFromBackend();
    void updateZoomPercent();
    void publish(VideoDocumentChange change);

    QObject* m_documentObject = nullptr;
    VideoDocumentState m_state;
    std::unique_ptr<VideoMediaBackend> m_mediaBackend;
    MediaBackendFactory m_mediaBackendFactory;
    VideoSourceLoadRuntime m_sourceLoadRuntime;
    VideoPlaybackSourceDevice m_playbackSourceDevice;
    VideoOutputRuntime m_outputRuntime;
    quint64 m_playbackGeneration = 0;
    quint64 m_acceptedPlaybackGeneration = 0;
};
}

#endif
