// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGERESULT_H
#define KIRIVIEW_DECODEDIMAGERESULT_H

#include "animationframe.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <QtGlobal>
#include <variant>
#include <vector>

namespace KiriView {
struct DecodedImageFailure {
    QString errorString;
};

struct StaticDecodedImage {
    QImage image;
};

struct SvgDecodedImage {
    QByteArray data;
    QSize svgIntrinsicSize;
};

struct DecodedAnimationImage {
    std::vector<AnimationFrame> frames;
    int loopCount = 0;
};

struct ReaderAnimationImage {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
    int loopCount = 0;
    int firstFrameDelay = 0;
};

struct HeifSequenceAnimationImage {
    QImage firstFrame;
    QByteArray data;
    int firstFrameDelay = 0;
};

using DecodedImageResult = std::variant<DecodedImageFailure, StaticDecodedImage, SvgDecodedImage,
    DecodedAnimationImage, ReaderAnimationImage, HeifSequenceAnimationImage>;

bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget);
}

#endif
