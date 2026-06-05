// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYER_H
#define KIRIVIEW_IMAGEANIMATIONPLAYER_H

#include "presentation/imageanimationplaybacksource.h"
#include "presentation/imageanimationpolicy.h"

#include <QImage>
#include <QTimer>
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
        ErrorCallback animationError, PlaybackStoppedCallback playbackStopped = {});
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
    std::unique_ptr<ImageAnimationPlaybackSource> m_source;
    QTimer m_timer;
    AnimationPlaybackState m_playbackState;
};
}

#endif
