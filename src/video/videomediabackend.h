// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOMEDIABACKEND_H
#define KIRIVIEW_VIDEOMEDIABACKEND_H

#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>

namespace kiriview {
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

struct VideoMediaBackendCallbacks
{
    std::function<void()> mediaStatusChanged;
    std::function<void()> errorChanged;
    std::function<void()> durationChanged;
    std::function<void()> positionChanged;
    std::function<void()> playingChanged;
    std::function<void()> seekableChanged;
    std::function<void()> hasVideoChanged;
    std::function<void()> hasAudioChanged;
    std::function<void()> videoSizeChanged;
    std::function<void()> mutedChanged;
    std::function<void()> videoOutputChanged;
};

class VideoMediaBackend
{
public:
    VideoMediaBackend() = default;

public:
    virtual ~VideoMediaBackend() = default;

    virtual void setCallbacks(VideoMediaBackendCallbacks callbacks) = 0;
    virtual void setSource(const QUrl& sourceUrl) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void setPosition(qint64 position) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void setVideoOutput(QObject* videoOutput) = 0;
    virtual QObject* videoOutput() const = 0;
    virtual VideoMediaStatus mediaStatus() const = 0;
    virtual QString errorString() const = 0;
    virtual qint64 duration() const = 0;
    virtual qint64 position() const = 0;
    virtual bool playing() const = 0;
    virtual bool seekable() const = 0;
    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
    virtual QSize videoSize() const = 0;
    virtual bool muted() const = 0;
    Q_DISABLE_COPY(VideoMediaBackend)
};

std::unique_ptr<VideoMediaBackend> createDefaultVideoMediaBackend(QObject* parent);
}

#endif
