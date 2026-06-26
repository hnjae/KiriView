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

QRectF imageTargetRect(QSize imageSize, QSizeF boundsSize);
QSize scaledImageSizeToFit(QSizeF imageSize, QSize boundsSize);
QSize firstDisplayScaledImageSize(QSize imageSize, QSize physicalViewportSize);
qreal imagePixelsPerSourcePixel(QSize imageSize, QSize displaySize);
QImage displayReadyImage(const QImage& image);
ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context);
ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    QSizeF viewportSize, qreal devicePixelRatio);
}

#endif
