// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERING_H
#define KIRIVIEW_IMAGERENDERING_H

#include "imagedocumenttypes.h"
#include "imagesurface.h"
#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>
#include <vector>

namespace KiriView {
inline constexpr int fallbackTextureSizeMax = 16384;

struct ImageSurfaceDrawEntry {
    QImage image;
    QRectF targetRect;
    QRectF textureRect;
};

QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize);
QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize);
std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect);
QImage displayReadyImage(const QImage &image);
QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize);
ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context);
ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    const QSizeF &viewportSize, qreal devicePixelRatio);
QImage renderSvgImage(const QByteArray &data, const QSize &size);
}

#endif
