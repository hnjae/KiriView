// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTRUNTIME_H
#define KIRIVIEW_VIDEODOCUMENTRUNTIME_H

#include "video/videodocumentstate.h"
#include "video/videomediabackend.h"
#include "video/videosourceloadruntime.h"

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
class VideoOutputRenderContextObserver;
class VideoDocumentRuntime final
{
public:
    using ChangeCallback = std::function<void(const std::vector<VideoDocumentChange> &)>;
    using MediaBackendFactory = std::function<std::unique_ptr<VideoMediaBackend>(QObject *)>;

    explicit VideoDocumentRuntime(QObject *documentObject, ChangeCallback changeCallback = {},
        std::unique_ptr<VideoMediaBackend> mediaBackend = {},
        std::unique_ptr<VideoPlaybackUrlResolver> playbackUrlResolver = {},
        MediaBackendFactory mediaBackendFactory = {});
    ~VideoDocumentRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    VideoDocumentStatus status() const;
    QString errorString() const;
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
    QObject *videoOutput() const;
    void setVideoOutput(QObject *videoOutput);
    void setVideoOutputGeometry(const QRectF &contentRect, const QRectF &sourceRect);

    void play();
    void pause();
    void stop();
    void togglePlayback();
    void seekBy(qint64 deltaMilliseconds);

    static qint64 clampedSeekPosition(
        qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable);

private:
    VideoMediaBackend *ensureMediaBackend();
    void installMediaBackendCallbacks();
    VideoSourceLoadOperations sourceLoadOperations();
    void clearPlaybackSource();
    void applyResolvedPlaybackUrl(const VideoPlaybackUrlResolution &resolution);
    void publishSourceLoadFailure(const QUrl &sourceUrl, const QString &errorString);
    void connectVideoOutputDestroyed(QObject *videoOutput);
    void disconnectVideoOutputDestroyed();
    void updateStatusFromBackend();
    void updateErrorFromBackend();
    void updateZoomPercent();
    void publish(VideoDocumentChange change);

    QObject *m_documentObject = nullptr;
    VideoDocumentState m_state;
    std::unique_ptr<VideoMediaBackend> m_mediaBackend;
    MediaBackendFactory m_mediaBackendFactory;
    VideoSourceLoadRuntime m_sourceLoadRuntime;
    std::unique_ptr<VideoOutputRenderContextObserver> m_renderContextObserver;
    QPointer<QObject> m_videoOutput;
    QRectF m_videoOutputContentRect;
    QRectF m_videoOutputSourceRect;
    QMetaObject::Connection m_videoOutputDestroyedConnection;
};
}

#endif
