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
class QtVideoMediaBackend final : public QObject, public KiriView::VideoMediaBackend
{
public:
    explicit QtVideoMediaBackend(QObject *parent)
        : QObject(parent)
        , m_player(this)
        , m_audioOutput(this)
    {
        m_player.setAudioOutput(&m_audioOutput);
        QObject::connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.mediaStatusChanged); });
        QObject::connect(&m_player, &QMediaPlayer::errorChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.errorChanged); });
        QObject::connect(&m_player, &QMediaPlayer::durationChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.durationChanged); });
        QObject::connect(&m_player, &QMediaPlayer::positionChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.positionChanged); });
        QObject::connect(&m_player, &QMediaPlayer::playingChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.playingChanged); });
        QObject::connect(&m_player, &QMediaPlayer::seekableChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.seekableChanged); });
        QObject::connect(&m_player, &QMediaPlayer::hasVideoChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.hasVideoChanged); });
        QObject::connect(&m_player, &QMediaPlayer::hasAudioChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.hasAudioChanged); });
        QObject::connect(&m_player, &QMediaPlayer::metaDataChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.videoSizeChanged); });
        QObject::connect(&m_player, &QMediaPlayer::videoOutputChanged, this,
            [this]() { KiriView::invokeIfSet(m_callbacks.videoOutputChanged); });
    }

    void setCallbacks(KiriView::VideoMediaBackendCallbacks callbacks) override
    {
        m_callbacks = std::move(callbacks);
    }

    void setSource(const QUrl &sourceUrl) override { m_player.setSource(sourceUrl); }
    void play() override { m_player.play(); }
    void pause() override { m_player.pause(); }
    void stop() override { m_player.stop(); }
    void setPosition(qint64 position) override { m_player.setPosition(position); }
    void setVideoOutput(QObject *videoOutput) override { m_player.setVideoOutput(videoOutput); }
    QObject *videoOutput() const override { return m_player.videoOutput(); }

    KiriView::VideoMediaStatus mediaStatus() const override
    {
        switch (m_player.mediaStatus()) {
        case QMediaPlayer::NoMedia:
            return KiriView::VideoMediaStatus::Null;
        case QMediaPlayer::LoadingMedia:
            return KiriView::VideoMediaStatus::Loading;
        case QMediaPlayer::LoadedMedia:
            return KiriView::VideoMediaStatus::Loaded;
        case QMediaPlayer::StalledMedia:
            return KiriView::VideoMediaStatus::Stalled;
        case QMediaPlayer::BufferingMedia:
            return KiriView::VideoMediaStatus::Buffering;
        case QMediaPlayer::BufferedMedia:
            return KiriView::VideoMediaStatus::Buffered;
        case QMediaPlayer::EndOfMedia:
            return KiriView::VideoMediaStatus::EndOfMedia;
        case QMediaPlayer::InvalidMedia:
            return KiriView::VideoMediaStatus::Invalid;
        }

        return KiriView::VideoMediaStatus::Null;
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

private:
    QMediaPlayer m_player;
    QAudioOutput m_audioOutput;
    KiriView::VideoMediaBackendCallbacks m_callbacks;
};
}

namespace KiriView {
std::unique_ptr<VideoMediaBackend> createDefaultVideoMediaBackend(QObject *parent)
{
    return std::make_unique<QtVideoMediaBackend>(parent);
}
}
