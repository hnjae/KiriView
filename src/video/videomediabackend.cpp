// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videomediabackend.h"

#include "async/imagecallback.h"

#include <QAudioOutput>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QVariant>
#include <algorithm>
#include <utility>

namespace {
class QtVideoMediaBackend final : public QObject, public kiriview::VideoMediaBackend
{
public:
    explicit QtVideoMediaBackend(QObject* parent)
        : QObject(parent)
        , m_player(this)
        , m_audioOutput(this)
    {
        m_player.setAudioOutput(&m_audioOutput);
        QObject::connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.mediaStatusChanged); });
        QObject::connect(&m_player, &QMediaPlayer::errorChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.errorChanged); });
        QObject::connect(&m_player, &QMediaPlayer::durationChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.durationChanged); });
        QObject::connect(&m_player, &QMediaPlayer::positionChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.positionChanged); });
        QObject::connect(&m_player, &QMediaPlayer::playingChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.playingChanged); });
        QObject::connect(&m_player, &QMediaPlayer::seekableChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.seekableChanged); });
        QObject::connect(&m_player, &QMediaPlayer::hasVideoChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.hasVideoChanged); });
        QObject::connect(&m_player, &QMediaPlayer::hasAudioChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.hasAudioChanged); });
        QObject::connect(&m_player, &QMediaPlayer::metaDataChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.videoSizeChanged); });
        QObject::connect(&m_audioOutput, &QAudioOutput::mutedChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.mutedChanged); });
        QObject::connect(&m_player, &QMediaPlayer::videoOutputChanged, this,
            [this]() { kiriview::invokeIfSet(m_callbacks.videoOutputChanged); });
    }

    void setCallbacks(kiriview::VideoMediaBackendCallbacks callbacks) override
    {
        m_callbacks = std::move(callbacks);
    }

    void setSource(const QUrl& sourceUrl) override { m_player.setSource(sourceUrl); }
    void play() override { m_player.play(); }
    void pause() override { m_player.pause(); }
    void stop() override { m_player.stop(); }
    void setPosition(qint64 position) override { m_player.setPosition(position); }
    void setMuted(bool muted) override { m_audioOutput.setMuted(muted); }
    void setVideoOutput(QObject* videoOutput) override { m_player.setVideoOutput(videoOutput); }
    QObject* videoOutput() const override { return m_player.videoOutput(); }

    kiriview::VideoMediaStatus mediaStatus() const override
    {
        switch (m_player.mediaStatus()) {
        case QMediaPlayer::NoMedia:
            return kiriview::VideoMediaStatus::Null;
        case QMediaPlayer::LoadingMedia:
            return kiriview::VideoMediaStatus::Loading;
        case QMediaPlayer::LoadedMedia:
            return kiriview::VideoMediaStatus::Loaded;
        case QMediaPlayer::StalledMedia:
            return kiriview::VideoMediaStatus::Stalled;
        case QMediaPlayer::BufferingMedia:
            return kiriview::VideoMediaStatus::Buffering;
        case QMediaPlayer::BufferedMedia:
            return kiriview::VideoMediaStatus::Buffered;
        case QMediaPlayer::EndOfMedia:
            return kiriview::VideoMediaStatus::EndOfMedia;
        case QMediaPlayer::InvalidMedia:
            return kiriview::VideoMediaStatus::Invalid;
        }

        return kiriview::VideoMediaStatus::Null;
    }

    QString errorString() const override { return m_player.errorString(); }
    qint64 duration() const override { return std::max<qint64>(0, m_player.duration()); }
    qint64 position() const override { return std::max<qint64>(0, m_player.position()); }
    bool playing() const override { return m_player.isPlaying(); }
    bool seekable() const override { return m_player.isSeekable(); }
    bool hasVideo() const override { return m_player.hasVideo(); }
    bool hasAudio() const override { return m_player.hasAudio(); }
    QSize videoSize() const override
    {
        return m_player.metaData().value(QMediaMetaData::Resolution).toSize();
    }
    bool muted() const override { return m_audioOutput.isMuted(); }

private:
    QMediaPlayer m_player;
    QAudioOutput m_audioOutput;
    kiriview::VideoMediaBackendCallbacks m_callbacks;
};
}

namespace kiriview {
std::unique_ptr<VideoMediaBackend> createDefaultVideoMediaBackend(QObject* parent)
{
    return std::make_unique<QtVideoMediaBackend>(parent);
}
}
