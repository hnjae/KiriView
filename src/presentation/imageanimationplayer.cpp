// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include "async/imagecallback.h"

#include <QObject>
#include <memory>
#include <utility>

namespace KiriView {
ImageAnimationPlayer::ImageAnimationPlayer(QObject *context, FrameReadyCallback frameReady,
    ErrorCallback animationError, PlaybackStoppedCallback playbackStopped)
    : m_frameReady(std::move(frameReady))
    , m_animationError(std::move(animationError))
    , m_playbackStopped(std::move(playbackStopped))
{
    m_timer.setSingleShot(true);
    QObject::connect(&m_timer, &QTimer::timeout, context, [this]() { advanceFrame(); });
}

ImageAnimationPlayer::~ImageAnimationPlayer() { stop(); }

void ImageAnimationPlayer::start(ImageAnimationPlaybackRequest request)
{
    start(makeImageAnimationPlaybackSource(std::move(request)));
}

void ImageAnimationPlayer::start(std::unique_ptr<ImageAnimationPlaybackSource> source)
{
    clearPlaybackState();
    if (source == nullptr) {
        return;
    }

    const ImageAnimationPlaybackOpenResult openResult = source->open();
    if (openResult.status == ImageAnimationPlaybackOpenStatus::Error) {
        finishWithError(openResult.errorString);
        return;
    }

    m_playbackState.startLoop(openResult.loopCount);
    m_source = std::move(source);
    if (openResult.sourceHasMoreFrames) {
        scheduleNextFrame(openResult.firstFrameDelay);
    }
}

void ImageAnimationPlayer::stop() { notifyStoppedIfActive(); }

void ImageAnimationPlayer::advanceFrame()
{
    if (m_source == nullptr) {
        return;
    }

    const ImageAnimationPlaybackReadResult readResult = m_source->readNextFrame();
    if (readResult.status == ImageAnimationPlaybackReadStatus::End) {
        handleSequenceEnd();
        return;
    }

    if (readResult.status == ImageAnimationPlaybackReadStatus::Error) {
        finishWithError(readResult.errorString);
        return;
    }

    invokeIfSet(m_frameReady, readResult.frame.image);
    applyFramePlan(
        m_playbackState.planAfterFrame(readResult.sourceHasMoreFrames), readResult.frame.delay);
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

void ImageAnimationPlayer::handleSequenceEnd()
{
    if (m_source == nullptr || !m_source->restartable()) {
        stop();
        return;
    }

    const AnimationSequencePlan sequencePlan = m_playbackState.planAtSequenceEnd();
    if (sequencePlan.action != AnimationSequenceAction::RestartSequence) {
        stop();
        return;
    }

    const ImageAnimationPlaybackOpenResult openResult = m_source->open();
    if (openResult.status == ImageAnimationPlaybackOpenStatus::Error) {
        finishWithError(openResult.errorString);
        return;
    }

    invokeIfSet(m_frameReady, openResult.firstFrame);
    applyFramePlan(
        m_playbackState.planAfterFrame(openResult.sourceHasMoreFrames), openResult.firstFrameDelay);
}

void ImageAnimationPlayer::clearPlaybackState()
{
    m_timer.stop();
    m_source.reset();
    m_playbackState.clear();
}

void ImageAnimationPlayer::notifyStoppedIfActive()
{
    const bool wasActive = m_source != nullptr || m_timer.isActive();
    clearPlaybackState();
    if (wasActive) {
        invokeIfSet(m_playbackStopped);
    }
}

void ImageAnimationPlayer::finishWithError(const QString &errorString)
{
    notifyStoppedIfActive();
    invokeIfSet(m_animationError, errorString);
}
}
