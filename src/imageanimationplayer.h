// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYER_H
#define KIRIVIEW_IMAGEANIMATIONPLAYER_H

#include "animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QTimer>
#include <cstddef>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

class QObject;
class QString;

namespace KiriView {
class BufferedImageReader;
class HeifSequenceReader;
class ImageAnimationPlayer
{
public:
    using FrameReadyCallback = std::function<void(const QImage &image)>;
    using ErrorCallback = std::function<void(const QString &errorString)>;

    ImageAnimationPlayer(
        QObject *context, FrameReadyCallback frameReady, ErrorCallback animationError);
    ~ImageAnimationPlayer();

    void start(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startDecoded(std::vector<AnimationFrame> frames, int loopCount);
    void startHeifSequence(const QByteArray &data);
    void stop();

private:
    struct ReaderPlayback {
        QByteArray data;
        QByteArray format;
        std::unique_ptr<BufferedImageReader> reader;
    };

    struct DecodedPlayback {
        std::vector<AnimationFrame> frames;
        std::size_t frameIndex = 0;
    };

    struct HeifSequencePlayback {
        QByteArray data;
        std::unique_ptr<HeifSequenceReader> reader;
    };

    using Playback
        = std::variant<std::monostate, ReaderPlayback, DecodedPlayback, HeifSequencePlayback>;

    void advanceFrame();
    void advanceReaderFrame(ReaderPlayback &playback);
    void advanceDecodedFrame(DecodedPlayback &playback);
    void advanceHeifSequenceFrame(HeifSequencePlayback &playback);
    bool resetReader(ReaderPlayback &playback, QString *errorString);
    bool resetHeifSequence(
        HeifSequencePlayback &playback, int *firstFrameDelay, QString *errorString);
    bool advanceLoopOrStop();
    bool hasRemainingLoops() const;
    void scheduleNextFrame(int delay);
    void scheduleNextFrameOrStop(bool shouldContinue, int delay);
    void clearPlaybackState();
    void finishWithError(const QString &errorString);

    FrameReadyCallback m_frameReady;
    ErrorCallback m_animationError;
    Playback m_playback;
    QTimer m_timer;
    int m_loopCount = 0;
    int m_completedLoops = 0;
};
}

#endif
