// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDECODER_H
#define KIRIVIEW_KIRIIMAGEDECODER_H

#include "apngdecoder.h"
#include "imagebytecost.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QtGlobal>
#include <vector>

namespace KiriView {
inline constexpr int fallbackTextureSizeMax = 16384;

struct DecodedImageResult {
    bool success = false;
    bool isSvg = false;
    QString errorString;
    QImage image;
    QByteArray svgData;
    QSize svgIntrinsicSize;
    std::vector<AnimationFrame> decodedAnimationFrames;
    QByteArray animationData;
    QByteArray animationFormat;
    int animationLoopCount = 0;
    int firstFrameDelay = 0;
    bool hasAnimationReaderFrames = false;
};

QImage displayReadyImage(const QImage &image);
QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize);
QImage renderSvgImage(const QByteArray &data, const QSize &size);
DecodedImageResult decodeImageData(const QByteArray &data);
bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget);
}

#endif
