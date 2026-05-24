// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H
#define KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H

#include "decoding/animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <memory>

namespace KiriView {
enum class ImageAnimationPlaybackOpenStatus {
    Success,
    Error,
};

struct ImageAnimationPlaybackOpenResult {
    ImageAnimationPlaybackOpenStatus status = ImageAnimationPlaybackOpenStatus::Error;
    QImage firstFrame;
    int firstFrameDelay = 0;
    int loopCount = 0;
    bool sourceHasMoreFrames = false;
    QString errorString;
};

enum class ImageAnimationPlaybackReadStatus {
    Frame,
    End,
    Error,
};

struct ImageAnimationPlaybackReadResult {
    ImageAnimationPlaybackReadStatus status = ImageAnimationPlaybackReadStatus::End;
    AnimationFrame frame;
    QString errorString;
};

class ImageAnimationPlaybackSource
{
public:
    virtual ~ImageAnimationPlaybackSource() = default;

    virtual ImageAnimationPlaybackOpenResult open() = 0;
    virtual ImageAnimationPlaybackReadResult readNextFrame() = 0;
    virtual bool hasMoreFrames() const = 0;
    virtual bool restartable() const = 0;
};

std::unique_ptr<ImageAnimationPlaybackSource> makeReaderAnimationPlaybackSource(
    QByteArray data, QByteArray format);
std::unique_ptr<ImageAnimationPlaybackSource> makeApngAnimationPlaybackSource(QByteArray data);
std::unique_ptr<ImageAnimationPlaybackSource> makeHeifSequenceAnimationPlaybackSource(
    QByteArray data);
}

#endif
