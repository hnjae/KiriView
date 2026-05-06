// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationplayer.h"

#include "bufferedimagereader.h"
#include "heifdecoder.h"
#include "imagecallback.h"
#include "imageviewtext.h"

#include <QObject>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace {
constexpr int defaultAnimationFrameDelay = 100;
constexpr int minimumAnimationFrameDelay = 10;

int normalizedAnimationFrameDelay(int delay)
{
    if (delay < 0) {
        return defaultAnimationFrameDelay;
    }

    return std::max(delay, minimumAnimationFrameDelay);
}
}

namespace KiriView {
ImageAnimationPlayer::ImageAnimationPlayer(
    QObject *context, FrameReadyCallback frameReady, ErrorCallback animationError)
    : m_frameReady(std::move(frameReady))
    , m_animationError(std::move(animationError))
{
    m_timer.setSingleShot(true);
    QObject::connect(&m_timer, &QTimer::timeout, context, [this]() { advanceFrame(); });
}

ImageAnimationPlayer::~ImageAnimationPlayer() { stop(); }

void ImageAnimationPlayer::start(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    clearPlaybackState();
    m_loopCount = loopCount;

    ReaderPlayback playback;
    playback.data = data;
    playback.format = format;

    QString errorString;
    if (!resetReader(playback, &errorString)) {
        finishWithError(errorString);
        return;
    }

    const QImage firstFrame = playback.reader->read();
    if (firstFrame.isNull()) {
        finishWithError(playback.reader->errorString());
        return;
    }

    const bool hasMoreFrames = playback.reader->canRead();
    m_playback = std::move(playback);
    if (hasMoreFrames) {
        scheduleNextFrame(firstFrameDelay);
    }
}

void ImageAnimationPlayer::startDecoded(std::vector<AnimationFrame> frames, int loopCount)
{
    clearPlaybackState();
    m_loopCount = loopCount;

    DecodedPlayback playback;
    playback.frames = std::move(frames);
    const bool hasMoreFrames = playback.frames.size() > 1;
    const int nextFrameDelay = hasMoreFrames ? playback.frames.front().delay : 0;

    m_playback = std::move(playback);
    if (hasMoreFrames) {
        scheduleNextFrame(nextFrameDelay);
    }
}

void ImageAnimationPlayer::startHeifSequence(const QByteArray &data)
{
    clearPlaybackState();
    HeifSequencePlayback playback;
    playback.data = data;

    QString errorString;
    int firstFrameDelay = 0;
    if (!resetHeifSequence(playback, &firstFrameDelay, &errorString)) {
        finishWithError(errorString);
        return;
    }

    m_playback = std::move(playback);
    scheduleNextFrame(firstFrameDelay);
}

void ImageAnimationPlayer::stop() { clearPlaybackState(); }

void ImageAnimationPlayer::advanceFrame()
{
    struct PlaybackVisitor {
        ImageAnimationPlayer *player = nullptr;

        void operator()(std::monostate &) const { }
        void operator()(ReaderPlayback &playback) const { player->advanceReaderFrame(playback); }
        void operator()(DecodedPlayback &playback) const { player->advanceDecodedFrame(playback); }
        void operator()(HeifSequencePlayback &playback) const
        {
            player->advanceHeifSequenceFrame(playback);
        }
    };

    std::visit(PlaybackVisitor { this }, m_playback);
}

void ImageAnimationPlayer::advanceReaderFrame(ReaderPlayback &playback)
{
    if (playback.reader == nullptr) {
        return;
    }

    if (!playback.reader->canRead()) {
        if (!advanceLoopOrStop()) {
            return;
        }

        QString errorString;
        if (!resetReader(playback, &errorString)) {
            finishWithError(errorString);
            return;
        }
    }

    QImage frame = playback.reader->read();
    if (frame.isNull()) {
        finishWithError(playback.reader->errorString());
        return;
    }

    const int delay = playback.reader->nextImageDelay();
    invokeIfSet(m_frameReady, frame);

    scheduleNextFrameOrStop(playback.reader->canRead() || hasRemainingLoops(), delay);
}

void ImageAnimationPlayer::advanceDecodedFrame(DecodedPlayback &playback)
{
    if (playback.frames.empty()) {
        return;
    }

    if (playback.frameIndex + 1 >= playback.frames.size()) {
        if (!advanceLoopOrStop()) {
            return;
        }

        playback.frameIndex = 0;
    } else {
        ++playback.frameIndex;
    }

    const AnimationFrame &frame = playback.frames.at(playback.frameIndex);
    invokeIfSet(m_frameReady, frame.image);

    scheduleNextFrameOrStop(
        playback.frameIndex + 1 < playback.frames.size() || hasRemainingLoops(), frame.delay);
}

void ImageAnimationPlayer::advanceHeifSequenceFrame(HeifSequencePlayback &playback)
{
    if (playback.reader == nullptr) {
        return;
    }

    QString errorString;
    std::optional<AnimationFrame> frame = playback.reader->readNextFrame(&errorString);
    if (!frame.has_value()) {
        if (errorString.isEmpty()) {
            stop();
        } else {
            finishWithError(errorString);
        }
        return;
    }

    invokeIfSet(m_frameReady, frame->image);
    scheduleNextFrame(frame->delay);
}

bool ImageAnimationPlayer::resetReader(ReaderPlayback &playback, QString *errorString)
{
    playback.reader.reset();

    auto reader = std::make_unique<BufferedImageReader>(playback.data, playback.format);
    if (!*reader) {
        *errorString = imageViewText("Could not read the selected image data.");
        return false;
    }

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    playback.reader = std::move(reader);
    return true;
}

bool ImageAnimationPlayer::resetHeifSequence(
    HeifSequencePlayback &playback, int *firstFrameDelay, QString *errorString)
{
    playback.reader = std::make_unique<HeifSequenceReader>();
    const HeifSequenceOpenResult openResult = playback.reader->open(playback.data);
    if (openResult.status != HeifSequenceOpenStatus::Success) {
        *errorString = openResult.errorString.isEmpty()
            ? imageViewText("Could not decode the selected HEIF image sequence.")
            : openResult.errorString;
        playback.reader.reset();
        return false;
    }

    std::optional<AnimationFrame> firstFrame = playback.reader->readNextFrame(errorString);
    if (!firstFrame.has_value()) {
        if (errorString->isEmpty()) {
            *errorString = imageViewText("Could not decode the selected HEIF image sequence.");
        }
        playback.reader.reset();
        return false;
    }

    *firstFrameDelay = firstFrame->delay;
    return true;
}

bool ImageAnimationPlayer::advanceLoopOrStop()
{
    if (!hasRemainingLoops()) {
        stop();
        return false;
    }

    ++m_completedLoops;
    return true;
}

bool ImageAnimationPlayer::hasRemainingLoops() const
{
    return m_loopCount < 0 || m_completedLoops < m_loopCount;
}

void ImageAnimationPlayer::scheduleNextFrame(int delay)
{
    m_timer.start(normalizedAnimationFrameDelay(delay));
}

void ImageAnimationPlayer::scheduleNextFrameOrStop(bool shouldContinue, int delay)
{
    if (shouldContinue) {
        scheduleNextFrame(delay);
        return;
    }

    stop();
}

void ImageAnimationPlayer::clearPlaybackState()
{
    m_timer.stop();
    m_playback = std::monostate();
    m_loopCount = 0;
    m_completedLoops = 0;
}

void ImageAnimationPlayer::finishWithError(const QString &errorString)
{
    invokeIfSet(m_animationError, errorString);
}
}
