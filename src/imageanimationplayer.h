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
    void startHeifSequence(const QByteArray &data, int firstFrameDelay);
    void stop();

private:
    void advanceFrame();
    void advanceReaderFrame();
    void advanceDecodedFrame();
    void advanceHeifSequenceFrame();
    bool resetReader(QString *errorString);
    bool resetHeifSequence(QString *errorString);
    bool hasRemainingLoops() const;
    void finishWithError(const QString &errorString);

    FrameReadyCallback m_frameReady;
    ErrorCallback m_animationError;
    QByteArray m_data;
    QByteArray m_format;
    std::vector<AnimationFrame> m_decodedFrames;
    std::unique_ptr<BufferedImageReader> m_reader;
    std::unique_ptr<HeifSequenceReader> m_heifSequenceReader;
    QTimer m_timer;
    std::size_t m_decodedFrameIndex = 0;
    int m_loopCount = 0;
    int m_completedLoops = 0;
    int m_heifFirstFrameDelay = 0;
};
}

#endif
