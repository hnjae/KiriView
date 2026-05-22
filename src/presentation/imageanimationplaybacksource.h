// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H
#define KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H

#include "decoding/animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <memory>
#include <optional>

namespace KiriView {
struct ImageAnimationPlaybackOpenResult {
    QImage firstFrame;
    int firstFrameDelay = 0;
    int loopCount = 0;
    bool sourceHasMoreFrames = false;
};

class ImageAnimationPlaybackSource
{
public:
    virtual ~ImageAnimationPlaybackSource() = default;

    virtual std::optional<ImageAnimationPlaybackOpenResult> open(QString *errorString) = 0;
    virtual std::optional<AnimationFrame> readNextFrame(QString *errorString) = 0;
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
