// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERING_H
#define KIRIVIEW_IMAGERENDERING_H

#include "imagerendercontext.h"
#include "rendering/imagerotation.h"
#include "staticimage.h"

#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>

namespace kiriview {
inline constexpr int fallbackTextureSizeMax = 16384;

QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize);
QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize);
QSize firstDisplayScaledImageSize(const QSize &imageSize, const QSize &physicalViewportSize);
qreal imagePixelsPerSourcePixel(const QSize &imageSize, const QSize &displaySize);
QImage displayReadyImage(const QImage &image);
ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context);
ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    const QSizeF &viewportSize, qreal devicePixelRatio);
}

#endif
