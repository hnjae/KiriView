// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYER_H
#define KIRIVIEW_IMAGEANIMATIONPLAYER_H

#include "async/timerscheduler.h"
#include "presentation/imageanimationplaybacksource.h"
#include "presentation/imageanimationpolicy.h"

#include <QImage>
#include <functional>
#include <memory>

class QObject;
class QString;

namespace KiriView {
class ImageAnimationPlayer
{
public:
    using FrameReadyCallback = std::function<void(const QImage &image)>;
    using ErrorCallback = std::function<void(const QString &errorString)>;
    using PlaybackStoppedCallback = std::function<void()>;

    ImageAnimationPlayer(QObject *context, FrameReadyCallback frameReady,
        ErrorCallback animationError, PlaybackStoppedCallback playbackStopped = {},
        TimerScheduler timerScheduler = {});
    ~ImageAnimationPlayer();

    void start(ImageAnimationPlaybackRequest request);
    void start(std::unique_ptr<ImageAnimationPlaybackSource> source);
    void stop();

private:
    void advanceFrame();
    void handleSequenceEnd();
    void scheduleNextFrame(int delay);
    void applyFramePlan(AnimationFramePlan plan, int delay);
    void clearPlaybackState();
    void notifyStoppedIfActive();
    void finishWithError(const QString &errorString);

    FrameReadyCallback m_frameReady;
    ErrorCallback m_animationError;
    PlaybackStoppedCallback m_playbackStopped;
    QObject *m_context = nullptr;
    TimerScheduler m_timerScheduler;
    std::unique_ptr<RuntimeTimerHandle> m_frameTimer;
    std::unique_ptr<ImageAnimationPlaybackSource> m_source;
    bool m_frameScheduled = false;
    AnimationPlaybackState m_playbackState;
};
}

#endif
