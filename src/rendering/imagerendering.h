// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGERENDERING_H
#define KIRIVIEW_IMAGERENDERING_H

#include "document/imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagesurfacedrawcontext.h"
#include "presentation/imagerotation.h"
#include "staticimage.h"

#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QtGlobal>
#include <vector>

namespace KiriView {
inline constexpr int fallbackTextureSizeMax = 16384;

enum class ImageSurfaceDrawIdentityKind {
    LegacyFrame,
    Preview,
    Tile,
};

struct ImageSurfaceDrawIdentity {
    ImageSurfaceDrawIdentityKind kind = ImageSurfaceDrawIdentityKind::LegacyFrame;
    TileKey tileKey;
    bool resolutionIndependent = false;

    friend bool operator==(
        const ImageSurfaceDrawIdentity &left, const ImageSurfaceDrawIdentity &right)
    {
        return left.kind == right.kind && left.tileKey == right.tileKey
            && left.resolutionIndependent == right.resolutionIndependent;
    }

    friend bool operator!=(
        const ImageSurfaceDrawIdentity &left, const ImageSurfaceDrawIdentity &right)
    {
        return !(left == right);
    }
};

struct ImageSurfaceDrawEntry {
    ImageSurfaceDrawIdentity identity;
    QImage image;
    QRectF targetRect;
    QRectF textureRect;
    ImageTextureCoordinateTransform textureTransform;
};

QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize);
QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize);
QSize firstDisplayScaledImageSize(const QSize &imageSize, const QSize &physicalViewportSize);
qreal imagePixelsPerSourcePixel(const QSize &imageSize, const QSize &displaySize);
std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const ImageSurfaceDrawContext &context);
std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect, int rotationDegrees = 0);
std::vector<ImageSurfaceDrawIdentity> imageSurfaceDrawIdentities(
    const std::vector<ImageSurfaceDrawEntry> &entries);
QImage displayReadyImage(const QImage &image);
ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context);
ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    const QSizeF &viewportSize, qreal devicePixelRatio);
}

#endif
