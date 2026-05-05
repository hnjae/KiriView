// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYER_H
#define KIRIVIEW_IMAGEANIMATIONPLAYER_H

#include "animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QTimer>
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
    struct ReaderPlayback;
    struct DecodedPlayback;
    struct HeifSequencePlayback;
    using Playback = std::variant<std::monostate, std::unique_ptr<ReaderPlayback>,
        std::unique_ptr<DecodedPlayback>, std::unique_ptr<HeifSequencePlayback>>;

    void advanceFrame();
    void advanceReaderFrame(ReaderPlayback &playback);
    void advanceDecodedFrame(DecodedPlayback &playback);
    void advanceHeifSequenceFrame(HeifSequencePlayback &playback);
    bool resetReader(ReaderPlayback &playback, QString *errorString);
    bool resetHeifSequence(
        HeifSequencePlayback &playback, int *firstFrameDelay, QString *errorString);
    bool hasRemainingLoops() const;
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
