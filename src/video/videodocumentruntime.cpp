// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include "async/imagecallback.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>
#include <algorithm>
#include <utility>

namespace {
QString fileNameForWindowTitle(const QUrl &sourceUrl)
{
    return sourceUrl.fileName(QUrl::PrettyDecoded);
}

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

    KiriView::VideoDocumentStatus mediaStatus() const override
    {
        switch (m_player.mediaStatus()) {
        case QMediaPlayer::NoMedia:
            return KiriView::VideoDocumentStatus::Null;
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::StalledMedia:
            return KiriView::VideoDocumentStatus::Loading;
        case QMediaPlayer::LoadedMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::BufferedMedia:
        case QMediaPlayer::EndOfMedia:
            return KiriView::VideoDocumentStatus::Ready;
        case QMediaPlayer::InvalidMedia:
            return KiriView::VideoDocumentStatus::Error;
        }

        return KiriView::VideoDocumentStatus::Null;
    }

    QString errorString() const override { return m_player.errorString(); }
    qint64 duration() const override { return std::max<qint64>(0, m_player.duration()); }
    qint64 position() const override { return std::max<qint64>(0, m_player.position()); }
    bool playing() const override { return m_player.isPlaying(); }
    bool seekable() const override { return m_player.isSeekable(); }
    bool hasVideo() const override { return m_player.hasVideo(); }
    bool hasAudio() const override { return m_player.hasAudio(); }

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

VideoDocumentRuntime::VideoDocumentRuntime(QObject *documentObject, ChangeCallback changeCallback,
    std::unique_ptr<VideoMediaBackend> mediaBackend,
    std::unique_ptr<VideoPlaybackUrlResolver> playbackUrlResolver,
    MediaBackendFactory mediaBackendFactory)
    : m_documentObject(documentObject)
    , m_changeCallback(std::move(changeCallback))
    , m_mediaBackend(std::move(mediaBackend))
    , m_mediaBackendFactory(std::move(mediaBackendFactory))
    , m_playbackUrlResolver(playbackUrlResolver == nullptr ? createDefaultVideoPlaybackUrlResolver()
                                                           : std::move(playbackUrlResolver))
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
        [this]() { setDurationValue(m_mediaBackend->duration()); },
        [this]() { setPositionValue(m_mediaBackend->position()); },
        [this]() { setPlayingValue(m_mediaBackend->playing()); },
        [this]() { setSeekableValue(m_mediaBackend->seekable()); },
        [this]() { setHasVideoValue(m_mediaBackend->hasVideo()); },
        [this]() { setHasAudioValue(m_mediaBackend->hasAudio()); },
        {},
    });
}

VideoDocumentRuntime::~VideoDocumentRuntime()
{
    disconnectVideoOutputDestroyed();
    if (m_playbackUrlResolver != nullptr) {
        m_playbackUrlResolver->cancel();
        m_playbackUrlResolver->cleanup();
    }
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setVideoOutput(nullptr);
        m_mediaBackend->setSource(QUrl());
    }
}

QUrl VideoDocumentRuntime::sourceUrl() const { return m_sourceUrl; }

void VideoDocumentRuntime::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    ++m_operationId;
    if (m_playbackUrlResolver != nullptr) {
        m_playbackUrlResolver->cancel();
        m_playbackUrlResolver->cleanup();
    }

    if (sourceUrl.isEmpty()) {
        clearSourceState();
        return;
    }

    beginSourceLoad(sourceUrl);
}

VideoDocumentStatus VideoDocumentRuntime::status() const { return m_status; }

QString VideoDocumentRuntime::errorString() const { return m_errorString; }

QString VideoDocumentRuntime::windowTitleFileName() const { return m_windowTitleFileName; }

qint64 VideoDocumentRuntime::duration() const { return m_duration; }

qint64 VideoDocumentRuntime::position() const { return m_position; }

void VideoDocumentRuntime::setPosition(qint64 position)
{
    if (!m_seekable) {
        return;
    }

    const qint64 upperBound = m_duration > 0 ? m_duration : position;
    const qint64 clamped = std::clamp(position, qint64(0), upperBound);
    ensureMediaBackend()->setPosition(clamped);
    setPositionValue(clamped);
}

bool VideoDocumentRuntime::playing() const { return m_playing; }

bool VideoDocumentRuntime::seekable() const { return m_seekable; }

bool VideoDocumentRuntime::hasVideo() const { return m_hasVideo; }

bool VideoDocumentRuntime::hasAudio() const { return m_hasAudio; }

QObject *VideoDocumentRuntime::videoOutput() const { return m_videoOutput.data(); }

void VideoDocumentRuntime::setVideoOutput(QObject *videoOutput)
{
    if (m_videoOutput.data() == videoOutput
        && (m_mediaBackend == nullptr || m_mediaBackend->videoOutput() == videoOutput)) {
        return;
    }

    disconnectVideoOutputDestroyed();
    m_videoOutput = videoOutput;
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->setVideoOutput(videoOutput);
    }
    connectVideoOutputDestroyed(videoOutput);
    publish(VideoDocumentChange::VideoOutput);
}

void VideoDocumentRuntime::play()
{
    if (m_sourceUrl.isEmpty()) {
        return;
    }

    ensureMediaBackend()->play();
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
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
    }
    setPlayingValue(false);
    if (m_seekable) {
        if (m_mediaBackend != nullptr) {
            m_mediaBackend->setPosition(0);
        }
        setPositionValue(0);
    }
}

void VideoDocumentRuntime::togglePlayback()
{
    if (m_playing) {
        pause();
        return;
    }

    play();
}

void VideoDocumentRuntime::seekBy(qint64 deltaMilliseconds)
{
    const qint64 nextPosition
        = clampedSeekPosition(m_position, deltaMilliseconds, m_duration, m_seekable);
    if (!m_seekable || nextPosition == m_position) {
        return;
    }

    ensureMediaBackend()->setPosition(nextPosition);
    setPositionValue(nextPosition);
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

void VideoDocumentRuntime::clearSourceState()
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
        m_mediaBackend->setSource(QUrl());
    }
    m_sourceUrl = QUrl();
    m_resolvingPlaybackUrl = false;

    std::vector<VideoDocumentChange> changes { VideoDocumentChange::SourceUrl };
    const auto appendChange = [&changes](VideoDocumentChange change) { changes.push_back(change); };

    if (m_status != VideoDocumentStatus::Null) {
        m_status = VideoDocumentStatus::Null;
        appendChange(VideoDocumentChange::Status);
    }
    if (!m_errorString.isEmpty()) {
        m_errorString.clear();
        appendChange(VideoDocumentChange::ErrorString);
    }
    if (!m_windowTitleFileName.isEmpty()) {
        m_windowTitleFileName.clear();
        appendChange(VideoDocumentChange::WindowTitleFileName);
    }
    if (m_duration != 0) {
        m_duration = 0;
        appendChange(VideoDocumentChange::Duration);
    }
    if (m_position != 0) {
        m_position = 0;
        appendChange(VideoDocumentChange::Position);
    }
    if (m_playing) {
        m_playing = false;
        appendChange(VideoDocumentChange::Playing);
    }
    if (m_seekable) {
        m_seekable = false;
        appendChange(VideoDocumentChange::Seekable);
    }
    if (m_hasVideo) {
        m_hasVideo = false;
        appendChange(VideoDocumentChange::HasVideo);
    }
    if (m_hasAudio) {
        m_hasAudio = false;
        appendChange(VideoDocumentChange::HasAudio);
    }

    publish(std::move(changes));
}

void VideoDocumentRuntime::beginSourceLoad(const QUrl &sourceUrl)
{
    if (m_mediaBackend != nullptr) {
        m_mediaBackend->stop();
        m_mediaBackend->setSource(QUrl());
    }
    m_sourceUrl = sourceUrl;
    m_resolvingPlaybackUrl = true;

    std::vector<VideoDocumentChange> changes {
        VideoDocumentChange::SourceUrl,
    };
    const QString fileName = fileNameForWindowTitle(sourceUrl);
    if (m_windowTitleFileName != fileName) {
        m_windowTitleFileName = fileName;
        changes.push_back(VideoDocumentChange::WindowTitleFileName);
    }
    if (m_errorString != QString()) {
        m_errorString.clear();
        changes.push_back(VideoDocumentChange::ErrorString);
    }
    if (m_status != VideoDocumentStatus::Loading) {
        m_status = VideoDocumentStatus::Loading;
        changes.push_back(VideoDocumentChange::Status);
    }
    if (m_duration != 0) {
        m_duration = 0;
        changes.push_back(VideoDocumentChange::Duration);
    }
    if (m_position != 0) {
        m_position = 0;
        changes.push_back(VideoDocumentChange::Position);
    }
    if (m_playing) {
        m_playing = false;
        changes.push_back(VideoDocumentChange::Playing);
    }
    if (m_seekable) {
        m_seekable = false;
        changes.push_back(VideoDocumentChange::Seekable);
    }
    if (m_hasVideo) {
        m_hasVideo = false;
        changes.push_back(VideoDocumentChange::HasVideo);
    }
    if (m_hasAudio) {
        m_hasAudio = false;
        changes.push_back(VideoDocumentChange::HasAudio);
    }
    publish(std::move(changes));

    const quint64 operationId = m_operationId;
    m_playbackUrlResolver->resolve(
        operationId, sourceUrl, m_documentObject,
        [this](
            VideoPlaybackUrlResolution resolution) { completePlaybackUrlResolution(resolution); },
        [this](quint64 failedOperationId, QUrl failedSourceUrl, QString failure) {
            failPlaybackUrlResolution(failedOperationId, failedSourceUrl, failure);
        });
}

void VideoDocumentRuntime::completePlaybackUrlResolution(
    const VideoPlaybackUrlResolution &resolution)
{
    if (resolution.operationId != m_operationId || resolution.sourceUrl != m_sourceUrl) {
        return;
    }

    m_resolvingPlaybackUrl = false;
    ensureMediaBackend()->setSource(resolution.playbackUrl);
    updateStatusFromBackend();
}

void VideoDocumentRuntime::failPlaybackUrlResolution(
    quint64 operationId, const QUrl &sourceUrl, const QString &errorString)
{
    if (operationId != m_operationId || sourceUrl != m_sourceUrl) {
        return;
    }

    m_resolvingPlaybackUrl = false;
    setErrorString(errorString);
    setStatus(VideoDocumentStatus::Error);
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
              publish(VideoDocumentChange::VideoOutput);
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
    if (m_sourceUrl.isEmpty()) {
        setStatus(VideoDocumentStatus::Null);
        return;
    }
    if (m_resolvingPlaybackUrl) {
        setStatus(VideoDocumentStatus::Loading);
        return;
    }
    if (m_mediaBackend == nullptr) {
        setStatus(VideoDocumentStatus::Loading);
        return;
    }

    const VideoDocumentStatus backendStatus = m_mediaBackend->mediaStatus();
    if (backendStatus == VideoDocumentStatus::Null) {
        setStatus(VideoDocumentStatus::Loading);
        return;
    }
    setStatus(backendStatus);
}

void VideoDocumentRuntime::updateErrorFromBackend()
{
    if (m_mediaBackend == nullptr) {
        return;
    }

    const QString backendError = m_mediaBackend->errorString();
    if (!backendError.isEmpty()) {
        setErrorString(backendError);
        setStatus(VideoDocumentStatus::Error);
    }
}

void VideoDocumentRuntime::setStatus(VideoDocumentStatus status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    publish(VideoDocumentChange::Status);
}

void VideoDocumentRuntime::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    publish(VideoDocumentChange::ErrorString);
}

void VideoDocumentRuntime::setDurationValue(qint64 duration)
{
    const qint64 normalizedDuration = std::max<qint64>(0, duration);
    if (m_duration == normalizedDuration) {
        return;
    }

    m_duration = normalizedDuration;
    publish(VideoDocumentChange::Duration);
}

void VideoDocumentRuntime::setPositionValue(qint64 position)
{
    const qint64 normalizedPosition = std::max<qint64>(0, position);
    if (m_position == normalizedPosition) {
        return;
    }

    m_position = normalizedPosition;
    publish(VideoDocumentChange::Position);
}

void VideoDocumentRuntime::setPlayingValue(bool playing)
{
    if (m_playing == playing) {
        return;
    }

    m_playing = playing;
    publish(VideoDocumentChange::Playing);
}

void VideoDocumentRuntime::setSeekableValue(bool seekable)
{
    if (m_seekable == seekable) {
        return;
    }

    m_seekable = seekable;
    publish(VideoDocumentChange::Seekable);
}

void VideoDocumentRuntime::setHasVideoValue(bool hasVideo)
{
    if (m_hasVideo == hasVideo) {
        return;
    }

    m_hasVideo = hasVideo;
    publish(VideoDocumentChange::HasVideo);
}

void VideoDocumentRuntime::setHasAudioValue(bool hasAudio)
{
    if (m_hasAudio == hasAudio) {
        return;
    }

    m_hasAudio = hasAudio;
    publish(VideoDocumentChange::HasAudio);
}

void VideoDocumentRuntime::publish(VideoDocumentChange change)
{
    publish(std::vector<VideoDocumentChange> { change });
}

void VideoDocumentRuntime::publish(std::vector<VideoDocumentChange> changes)
{
    if (changes.empty()) {
        return;
    }

    std::vector<VideoDocumentChange> uniqueChanges;
    for (VideoDocumentChange change : changes) {
        if (std::find(uniqueChanges.cbegin(), uniqueChanges.cend(), change)
            == uniqueChanges.cend()) {
            uniqueChanges.push_back(change);
        }
    }
    invokeIfSet(m_changeCallback, uniqueChanges);
}
}
