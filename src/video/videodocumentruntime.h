// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTRUNTIME_H
#define KIRIVIEW_VIDEODOCUMENTRUNTIME_H

#include "video/videodocumentstate.h"
#include "video/videosourceloadruntime.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <vector>

namespace KiriView {
enum class VideoMediaStatus {
    Null,
    Loading,
    Loaded,
    Stalled,
    Buffering,
    Buffered,
    EndOfMedia,
    Invalid,
};

struct VideoMediaBackendCallbacks {
    std::function<void()> mediaStatusChanged;
    std::function<void()> errorChanged;
    std::function<void()> durationChanged;
    std::function<void()> positionChanged;
    std::function<void()> playingChanged;
    std::function<void()> seekableChanged;
    std::function<void()> hasVideoChanged;
    std::function<void()> hasAudioChanged;
    std::function<void()> videoOutputChanged;
};

class VideoMediaBackend
{
public:
    virtual ~VideoMediaBackend() = default;

    virtual void setCallbacks(VideoMediaBackendCallbacks callbacks) = 0;
    virtual void setSource(const QUrl &sourceUrl) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void setPosition(qint64 position) = 0;
    virtual void setVideoOutput(QObject *videoOutput) = 0;
    virtual QObject *videoOutput() const = 0;
    virtual VideoMediaStatus mediaStatus() const = 0;
    virtual QString errorString() const = 0;
    virtual qint64 duration() const = 0;
    virtual qint64 position() const = 0;
    virtual bool playing() const = 0;
    virtual bool seekable() const = 0;
    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
};

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
    QObject *videoOutput() const;
    void setVideoOutput(QObject *videoOutput);

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
    void publish(VideoDocumentChange change);

    QObject *m_documentObject = nullptr;
    VideoDocumentState m_state;
    std::unique_ptr<VideoMediaBackend> m_mediaBackend;
    MediaBackendFactory m_mediaBackendFactory;
    VideoSourceLoadRuntime m_sourceLoadRuntime;
    QPointer<QObject> m_videoOutput;
    QMetaObject::Connection m_videoOutputDestroyedConnection;
};

std::unique_ptr<VideoMediaBackend> createDefaultVideoMediaBackend(QObject *parent);
}

#endif
