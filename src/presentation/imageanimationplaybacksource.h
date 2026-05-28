// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H
#define KIRIVIEW_IMAGEANIMATIONPLAYBACKSOURCE_H

#include "decoding/animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QString>
#include <memory>
#include <variant>

namespace KiriView {
struct ReaderAnimationPlaybackRequest {
    QByteArray data;
    QByteArray format;
};

struct ApngAnimationPlaybackRequest {
    QByteArray data;
};

struct HeifSequenceAnimationPlaybackRequest {
    QByteArray data;
};

struct ImageAnimationPlaybackRequest {
    using Payload = std::variant<std::monostate, ReaderAnimationPlaybackRequest,
        ApngAnimationPlaybackRequest, HeifSequenceAnimationPlaybackRequest>;

    Payload payload;

    bool isValid() const;
};

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
    bool sourceHasMoreFrames = false;
    QString errorString;
};

class ImageAnimationPlaybackSource
{
public:
    virtual ~ImageAnimationPlaybackSource() = default;

    virtual ImageAnimationPlaybackOpenResult open() = 0;
    virtual ImageAnimationPlaybackReadResult readNextFrame() = 0;
    virtual bool restartable() const = 0;
};

ImageAnimationPlaybackRequest readerAnimationPlaybackRequest(QByteArray data, QByteArray format);
ImageAnimationPlaybackRequest apngAnimationPlaybackRequest(QByteArray data);
ImageAnimationPlaybackRequest heifSequenceAnimationPlaybackRequest(QByteArray data);
std::unique_ptr<ImageAnimationPlaybackSource> makeImageAnimationPlaybackSource(
    ImageAnimationPlaybackRequest request);
}

#endif
