// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include "async/imagecallback.h"
#include "presentation/animationlogging.h"

#include <QDebug>
#include <QObject>
#include <memory>
#include <utility>

namespace kiriview {
ImageAnimationPlayer::ImageAnimationPlayer(QObject* context, FrameReadyCallback frameReady,
    ErrorCallback animationError, PlaybackStoppedCallback playbackStopped,
    TimerScheduler timerScheduler)
    : m_frameReady(std::move(frameReady))
    , m_animationError(std::move(animationError))
    , m_playbackStopped(std::move(playbackStopped))
    , m_context(context)
    , m_timerScheduler(timerSchedulerWithDefaults(std::move(timerScheduler)))
{
    m_frameTimer = m_timerScheduler.singleShotTimer(m_context, 0, [this]() {
        if (kiriviewAnimationLog().isDebugEnabled() && m_frameScheduleTimer.isValid()) {
            const qint64 elapsedMs = m_frameScheduleTimer.elapsed();
            qCDebug(kiriviewAnimationLog)
                << "animation frame timer fired" << "scheduledDelayMs" << m_scheduledFrameDelayMs
                << "elapsedMs" << elapsedMs << "driftMs" << elapsedMs - m_scheduledFrameDelayMs;
        }
        m_frameScheduled = false;
        advanceFrame();
    });
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
        qCDebug(kiriviewAnimationLog) << "animation start ignored for null source";
        return;
    }

    const ImageAnimationPlaybackOpenResult openResult = source->open();
    if (openResult.status == ImageAnimationPlaybackOpenStatus::Error) {
        qCDebug(kiriviewAnimationLog) << "animation open error" << openResult.errorString;
        finishWithError(openResult.errorString);
        return;
    }

    qCDebug(kiriviewAnimationLog) << "animation opened" << "loopCount" << openResult.loopCount
                                  << "firstFrameDelayMs" << openResult.firstFrameDelay
                                  << "sourceHasMoreFrames" << openResult.sourceHasMoreFrames
                                  << "firstFrameSize" << openResult.firstFrame.size();
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
        qCDebug(kiriviewAnimationLog) << "animation frame advance ignored without source";
        return;
    }

    const ImageAnimationPlaybackReadResult readResult = m_source->readNextFrame();
    if (readResult.status == ImageAnimationPlaybackReadStatus::End) {
        qCDebug(kiriviewAnimationLog) << "animation frame advance reached sequence end";
        handleSequenceEnd();
        return;
    }

    if (readResult.status == ImageAnimationPlaybackReadStatus::Error) {
        qCDebug(kiriviewAnimationLog) << "animation frame advance error" << readResult.errorString;
        finishWithError(readResult.errorString);
        return;
    }

    qCDebug(kiriviewAnimationLog) << "animation frame advanced" << "frameSize"
                                  << readResult.frame.image.size() << "frameDelayMs"
                                  << readResult.frame.delay << "sourceHasMoreFrames"
                                  << readResult.sourceHasMoreFrames;
    invokeIfSet(m_frameReady, readResult.frame.image);
    applyFramePlan(
        m_playbackState.planAfterFrame(readResult.sourceHasMoreFrames), readResult.frame.delay);
}

void ImageAnimationPlayer::scheduleNextFrame(int delay)
{
    if (m_frameTimer == nullptr) {
        m_frameTimer = m_timerScheduler.singleShotTimer(m_context, 0, [this]() {
            if (kiriviewAnimationLog().isDebugEnabled() && m_frameScheduleTimer.isValid()) {
                const qint64 elapsedMs = m_frameScheduleTimer.elapsed();
                qCDebug(kiriviewAnimationLog) << "animation frame timer fired" << "scheduledDelayMs"
                                              << m_scheduledFrameDelayMs << "elapsedMs" << elapsedMs
                                              << "driftMs" << elapsedMs - m_scheduledFrameDelayMs;
            }
            m_frameScheduled = false;
            advanceFrame();
        });
    }
    const int normalizedDelay = normalizedAnimationFrameDelay(delay);
    m_frameScheduled = true;
    m_scheduledFrameDelayMs = normalizedDelay;
    if (kiriviewAnimationLog().isDebugEnabled()) {
        m_frameScheduleTimer.restart();
    } else {
        m_frameScheduleTimer.invalidate();
    }
    qCDebug(kiriviewAnimationLog) << "animation frame scheduled" << "requestedDelayMs" << delay
                                  << "scheduledDelayMs" << normalizedDelay;
    m_frameTimer->start(normalizedDelay);
}

void ImageAnimationPlayer::applyFramePlan(AnimationFramePlan plan, int delay)
{
    switch (plan.action) {
    case AnimationFrameAction::ScheduleNextFrame:
        scheduleNextFrame(delay);
        return;
    case AnimationFrameAction::Stop:
        qCDebug(kiriviewAnimationLog)
            << "animation frame plan stopped" << "completedLoops" << plan.completedLoops;
        stop();
        return;
    }
}

void ImageAnimationPlayer::handleSequenceEnd()
{
    if (m_source == nullptr || !m_source->restartable()) {
        qCDebug(kiriviewAnimationLog) << "animation sequence ended" << "restartable"
                                      << (m_source != nullptr && m_source->restartable());
        stop();
        return;
    }

    const AnimationSequencePlan sequencePlan = m_playbackState.planAtSequenceEnd();
    if (sequencePlan.action != AnimationSequenceAction::RestartSequence) {
        qCDebug(kiriviewAnimationLog)
            << "animation sequence ended" << "completedLoops" << sequencePlan.completedLoops;
        stop();
        return;
    }

    qCDebug(kiriviewAnimationLog) << "animation sequence restarting" << "completedLoops"
                                  << sequencePlan.completedLoops;
    const ImageAnimationPlaybackOpenResult openResult = m_source->open();
    if (openResult.status == ImageAnimationPlaybackOpenStatus::Error) {
        qCDebug(kiriviewAnimationLog) << "animation restart open error" << openResult.errorString;
        finishWithError(openResult.errorString);
        return;
    }

    qCDebug(kiriviewAnimationLog) << "animation sequence restarted" << "firstFrameDelayMs"
                                  << openResult.firstFrameDelay << "sourceHasMoreFrames"
                                  << openResult.sourceHasMoreFrames << "firstFrameSize"
                                  << openResult.firstFrame.size();
    invokeIfSet(m_frameReady, openResult.firstFrame);
    applyFramePlan(
        m_playbackState.planAfterFrame(openResult.sourceHasMoreFrames), openResult.firstFrameDelay);
}

void ImageAnimationPlayer::clearPlaybackState()
{
    if (m_frameTimer != nullptr) {
        m_frameTimer->stop();
    }
    m_frameScheduled = false;
    m_scheduledFrameDelayMs = 0;
    m_frameScheduleTimer.invalidate();
    m_source.reset();
    m_playbackState.clear();
}

void ImageAnimationPlayer::notifyStoppedIfActive()
{
    const bool wasActive = m_source != nullptr || m_frameScheduled;
    clearPlaybackState();
    if (wasActive) {
        invokeIfSet(m_playbackStopped);
    }
}

void ImageAnimationPlayer::finishWithError(const QString& errorString)
{
    qCDebug(kiriviewAnimationLog) << "animation error" << errorString;
    notifyStoppedIfActive();
    invokeIfSet(m_animationError, errorString);
}
}
