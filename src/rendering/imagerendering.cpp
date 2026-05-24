// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "bridge/qtgeometryconversion.h"
#include "imagerenderframe.h"
#include "kiriview/src/policy/imagerendergeometry.cxx.h"

#include <QRectF>
#include <vector>

namespace KiriView {
QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize)
{
    return Bridge::qtRectF(rustImageTargetRect(Bridge::rustSize<RustImageRenderSize>(imageSize),
        Bridge::rustSizeF<RustImageRenderSizeF>(boundsSize)));
}

QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize)
{
    return Bridge::qtSize(
        rustScaledImageSizeToFit(Bridge::rustSizeF<RustImageRenderSizeF>(imageSize),
            Bridge::rustSize<RustImageRenderSize>(boundsSize)));
}

QSize firstDisplayScaledImageSize(const QSize &imageSize, const QSize &physicalViewportSize)
{
    return Bridge::qtSize(
        rustFirstDisplayScaledImageSize(Bridge::rustSize<RustImageRenderSize>(imageSize),
            Bridge::rustSize<RustImageRenderSize>(physicalViewportSize)));
}

qreal imagePixelsPerSourcePixel(const QSize &imageSize, const QSize &displaySize)
{
    return rustImagePixelsPerSourcePixel(Bridge::rustSize<RustImageRenderSize>(imageSize),
        Bridge::rustSize<RustImageRenderSize>(displaySize));
}

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const ImageSurfaceDrawContext &context)
{
    return projectImageRenderFrame(
        ImageRenderFrameInput { &surface, 0, context, DisplayedPageRole::Primary, {} })
        .drawEntries;
}

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect, int rotationDegrees)
{
    return imageSurfaceDrawEntries(surface,
        ImageSurfaceDrawContext {
            targetRect,
            targetRect.size(),
            targetRect,
            1.0,
            rotationDegrees,
        });
}

std::vector<ImageSurfaceDrawIdentity> imageSurfaceDrawIdentities(
    const std::vector<ImageSurfaceDrawEntry> &entries)
{
    std::vector<ImageSurfaceDrawIdentity> identities;
    identities.reserve(entries.size());
    for (const ImageSurfaceDrawEntry &entry : entries) {
        identities.push_back(entry.identity);
    }
    return identities;
}

QImage displayReadyImage(const QImage &image)
{
    if (image.format() == QImage::Format_RGBA8888_Premultiplied) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
}

ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context)
{
    const RustImageDocumentRenderContext normalized = rustNormalizedImageDocumentRenderContext(
        RustImageDocumentRenderContext { context.devicePixelRatio, context.maximumTextureSize },
        fallbackTextureSizeMax);
    return ImageDocumentRenderContext {
        normalized.device_pixel_ratio,
        normalized.maximum_texture_size,
    };
}

ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    const QSizeF &viewportSize, qreal devicePixelRatio)
{
    return ImageFirstDisplayDecodeContext {
        Bridge::qtSize(rustFirstDisplayPhysicalViewportSize(
            Bridge::rustSizeF<RustImageRenderSizeF>(viewportSize), devicePixelRatio)),
    };
}
}
