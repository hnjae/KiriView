// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYER_H
#define KIRIVIEW_IMAGEANIMATIONPLAYER_H

#include "apngdecoder.h"

#include <QByteArray>
#include <QImage>
#include <QTimer>
#include <functional>
#include <memory>
#include <vector>

class QBuffer;
class QObject;
class QImageReader;
class QString;

namespace KiriView {
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
    void stop();

private:
    void advanceFrame();
    void advanceReaderFrame();
    void advanceDecodedFrame();
    bool resetReader(QString *errorString);
    bool hasRemainingLoops() const;
    void finishWithError(const QString &errorString);

    FrameReadyCallback m_frameReady;
    ErrorCallback m_animationError;
    QByteArray m_data;
    QByteArray m_format;
    std::vector<AnimationFrame> m_decodedFrames;
    std::unique_ptr<QBuffer> m_buffer;
    std::unique_ptr<QImageReader> m_reader;
    QTimer m_timer;
    std::size_t m_decodedFrameIndex = 0;
    int m_loopCount = 0;
    int m_completedLoops = 0;
};
}

#endif
