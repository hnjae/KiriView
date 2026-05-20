// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include "async/imagecallback.h"
#include "decoding/apnganimationreader.h"
#include "decoding/bufferedimagereader.h"
#include "decoding/heifsequencereader.h"
#include "localization/imageerrortext.h"

#include <QObject>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

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

void ImageAnimationPlayer::start(const QByteArray &data, const QByteArray &format)
{
    clearPlaybackState();

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

    const int firstFrameDelay = playback.reader->nextImageDelay();
    m_playbackState.startLoop(playback.reader->loopCount());
    const bool hasMoreFrames = playback.reader->canRead();
    m_playback = std::move(playback);
    if (hasMoreFrames) {
        scheduleNextFrame(firstFrameDelay);
    }
}

void ImageAnimationPlayer::startApng(const QByteArray &data)
{
    clearPlaybackState();

    ApngPlayback playback;
    playback.data = data;

    QString errorString;
    std::optional<ApngOpenResult> openResult = resetApng(playback, &errorString);
    if (!openResult.has_value()) {
        finishWithError(errorString);
        return;
    }

    const bool hasMoreFrames = openResult->frameCount > 1;
    m_playbackState.startLoop(openResult->loopCount);
    m_playback = std::move(playback);
    if (hasMoreFrames) {
        scheduleNextFrame(openResult->firstFrameDelay);
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
        void operator()(ApngPlayback &playback) const { player->advanceApngFrame(playback); }
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
        const AnimationSequencePlan sequencePlan = m_playbackState.planAtSequenceEnd();
        if (sequencePlan.action != AnimationSequenceAction::RestartSequence) {
            stop();
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

    applyFramePlan(m_playbackState.planAfterFrame(playback.reader->canRead()), delay);
}

void ImageAnimationPlayer::advanceApngFrame(ApngPlayback &playback)
{
    if (playback.reader == nullptr) {
        return;
    }

    QString errorString;
    std::optional<AnimationFrame> frame = playback.reader->readNextFrame(&errorString);
    if (!frame.has_value()) {
        if (!errorString.isEmpty()) {
            finishWithError(errorString);
            return;
        }

        const AnimationSequencePlan sequencePlan = m_playbackState.planAtSequenceEnd();
        if (sequencePlan.action != AnimationSequenceAction::RestartSequence) {
            stop();
            return;
        }

        std::optional<ApngOpenResult> openResult = resetApng(playback, &errorString);
        if (!openResult.has_value()) {
            finishWithError(errorString);
            return;
        }

        invokeIfSet(m_frameReady, openResult->firstFrame);
        applyFramePlan(m_playbackState.planAfterFrame(openResult->frameCount > 1),
            openResult->firstFrameDelay);
        return;
    }

    invokeIfSet(m_frameReady, frame->image);
    applyFramePlan(m_playbackState.planAfterFrame(playback.reader->hasMoreFrames()), frame->delay);
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
        *errorString = imageErrorText(ImageErrorTextId::ReadImageData);
        return false;
    }

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    playback.reader = std::move(reader);
    return true;
}

std::optional<ApngOpenResult> ImageAnimationPlayer::resetApng(
    ApngPlayback &playback, QString *errorString)
{
    playback.reader = std::make_unique<ApngAnimationReader>();
    ApngOpenResult openResult = playback.reader->open(playback.data);
    switch (openResult.status) {
    case ApngOpenStatus::Success:
        return openResult;
    case ApngOpenStatus::Error:
        *errorString = openResult.errorString;
        playback.reader.reset();
        return std::nullopt;
    case ApngOpenStatus::NotApng:
        *errorString = imageErrorText(ImageErrorTextId::DecodeApngAnimation);
        playback.reader.reset();
        return std::nullopt;
    }

    *errorString = imageErrorText(ImageErrorTextId::DecodeApngAnimation);
    playback.reader.reset();
    return std::nullopt;
}

bool ImageAnimationPlayer::resetHeifSequence(
    HeifSequencePlayback &playback, int *firstFrameDelay, QString *errorString)
{
    playback.reader = std::make_unique<HeifSequenceReader>();
    const HeifSequenceOpenResult openResult = playback.reader->open(playback.data);
    if (openResult.status != HeifSequenceOpenStatus::Success) {
        *errorString = openResult.errorString.isEmpty() ? heifSequenceDecodeErrorString()
                                                        : openResult.errorString;
        playback.reader.reset();
        return false;
    }

    std::optional<AnimationFrame> firstFrame = playback.reader->readNextFrame(errorString);
    if (!firstFrame.has_value()) {
        if (errorString->isEmpty()) {
            *errorString = heifSequenceDecodeErrorString();
        }
        playback.reader.reset();
        return false;
    }

    *firstFrameDelay = firstFrame->delay;
    return true;
}

void ImageAnimationPlayer::scheduleNextFrame(int delay)
{
    m_timer.start(normalizedAnimationFrameDelay(delay));
}

void ImageAnimationPlayer::applyFramePlan(AnimationFramePlan plan, int delay)
{
    switch (plan.action) {
    case AnimationFrameAction::ScheduleNextFrame:
        scheduleNextFrame(delay);
        return;
    case AnimationFrameAction::Stop:
        stop();
        return;
    }
}

void ImageAnimationPlayer::clearPlaybackState()
{
    m_timer.stop();
    m_playback = std::monostate();
    m_playbackState.clear();
}

void ImageAnimationPlayer::finishWithError(const QString &errorString)
{
    invokeIfSet(m_animationError, errorString);
}
}
