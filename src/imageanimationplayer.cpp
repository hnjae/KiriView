// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationplayer.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QIODevice>
#include <QImageReader>
#include <QObject>
#include <algorithm>
#include <utility>

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
    m_data = data;
    m_format = format;
    m_loopCount = loopCount;
    m_completedLoops = 0;

    QString errorString;
    if (!resetReader(&errorString)) {
        finishWithError(errorString);
        return;
    }

    const QImage firstFrame = m_reader->read();
    if (firstFrame.isNull()) {
        finishWithError(m_reader->errorString());
        return;
    }

    if (m_reader->canRead()) {
        m_timer.start(normalizedAnimationFrameDelay(firstFrameDelay));
    }
}

void ImageAnimationPlayer::startDecoded(std::vector<AnimationFrame> frames, int loopCount)
{
    m_decodedFrames = std::move(frames);
    m_decodedFrameIndex = 0;
    m_loopCount = loopCount;
    m_completedLoops = 0;

    if (m_decodedFrames.size() > 1) {
        m_timer.start(normalizedAnimationFrameDelay(m_decodedFrames.front().delay));
    }
}

void ImageAnimationPlayer::stop()
{
    m_timer.stop();
    m_reader.reset();
    m_buffer.reset();
    m_data.clear();
    m_format.clear();
    m_decodedFrames.clear();
    m_decodedFrameIndex = 0;
    m_loopCount = 0;
    m_completedLoops = 0;
}

void ImageAnimationPlayer::advanceFrame()
{
    if (!m_decodedFrames.empty()) {
        advanceDecodedFrame();
        return;
    }

    advanceReaderFrame();
}

void ImageAnimationPlayer::advanceReaderFrame()
{
    if (m_reader == nullptr) {
        return;
    }

    if (!m_reader->canRead()) {
        if (!hasRemainingLoops()) {
            stop();
            return;
        }

        ++m_completedLoops;

        QString errorString;
        if (!resetReader(&errorString)) {
            finishWithError(errorString);
            return;
        }
    }

    QImage frame = m_reader->read();
    if (frame.isNull()) {
        finishWithError(m_reader->errorString());
        return;
    }

    const int delay = normalizedAnimationFrameDelay(m_reader->nextImageDelay());
    m_frameReady(frame);

    if (m_reader->canRead() || hasRemainingLoops()) {
        m_timer.start(delay);
    } else {
        stop();
    }
}

void ImageAnimationPlayer::advanceDecodedFrame()
{
    if (m_decodedFrames.empty()) {
        return;
    }

    if (m_decodedFrameIndex + 1 >= m_decodedFrames.size()) {
        if (!hasRemainingLoops()) {
            stop();
            return;
        }

        ++m_completedLoops;
        m_decodedFrameIndex = 0;
    } else {
        ++m_decodedFrameIndex;
    }

    const AnimationFrame &frame = m_decodedFrames.at(m_decodedFrameIndex);
    m_frameReady(frame.image);

    if (m_decodedFrameIndex + 1 < m_decodedFrames.size() || hasRemainingLoops()) {
        m_timer.start(normalizedAnimationFrameDelay(frame.delay));
    } else {
        stop();
    }
}

bool ImageAnimationPlayer::resetReader(QString *errorString)
{
    m_reader.reset();
    m_buffer.reset();

    auto buffer = std::make_unique<QBuffer>();
    buffer->setData(m_data);

    if (!buffer->open(QIODevice::ReadOnly)) {
        *errorString = QCoreApplication::translate(
            "KiriImageView", "Could not read the selected image data.");
        return false;
    }

    auto reader = std::make_unique<QImageReader>(buffer.get(), m_format);
    reader->setAutoTransform(true);

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    m_buffer = std::move(buffer);
    m_reader = std::move(reader);
    return true;
}

bool ImageAnimationPlayer::hasRemainingLoops() const
{
    return m_loopCount < 0 || m_completedLoops < m_loopCount;
}

void ImageAnimationPlayer::finishWithError(const QString &errorString)
{
    m_animationError(errorString);
}
}
