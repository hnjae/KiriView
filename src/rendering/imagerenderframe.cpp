// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerenderframe.h"

#include "bridge/qtgeometryconversion.h"
#include "imagetilegeometrypolicy.h"
#include "imagetilevisibility.h"
#include "kiriview/src/policy/imagerendergeometry.cxx.h"
#include "rendering/imagerotation.h"

#include <QRect>
#include <QRectF>
#include <optional>
#include <utility>

namespace {
KiriView::ImageSurfaceDrawEntry drawEntry(KiriView::ImageSurfaceDrawIdentity identity,
    const QImage &image, const QRectF &targetRect, const QRectF &textureRect, int rotationDegrees)
{
    return KiriView::ImageSurfaceDrawEntry {
        identity,
        image,
        targetRect,
        textureRect,
        KiriView::imageTextureCoordinateTransform(textureRect, rotationDegrees),
    };
}

KiriView::ImageSurfaceDrawEntry fullImageDrawEntry(KiriView::ImageSurfaceDrawIdentity identity,
    const QImage &image, const QRectF &targetRect, int rotationDegrees)
{
    return drawEntry(identity, image, targetRect, QRectF(0.0, 0.0, 1.0, 1.0), rotationDegrees);
}

QRectF tileTargetRect(
    const QRectF &sourceRect, const QSize &imageSize, const QRectF &targetRect, int rotationDegrees)
{
    if (KiriView::normalizedImageRotationDegrees(rotationDegrees) != 0) {
        return KiriView::rotatedSourceRectInTarget(
            sourceRect, QSizeF(imageSize), targetRect, rotationDegrees);
    }

    return KiriView::Bridge::qtRectF(KiriView::rustImageTileTargetRectF(
        KiriView::Bridge::rustRectF<KiriView::RustImageRenderRectF>(sourceRect),
        KiriView::Bridge::rustSize<KiriView::RustImageRenderSize>(imageSize),
        KiriView::Bridge::rustRectF<KiriView::RustImageRenderRectF>(targetRect)));
}

QRectF tileTextureRect(const KiriView::DecodedTile &tile)
{
    return KiriView::Bridge::qtRectF(KiriView::rustImageTileTextureRect(
        KiriView::Bridge::rustRect<KiriView::RustImageRenderRect>(tile.levelRect),
        KiriView::Bridge::rustRect<KiriView::RustImageRenderRect>(tile.textureLevelRect)));
}

std::optional<KiriView::ImageSurfaceDrawEntry> staticTileDrawEntry(
    const KiriView::StaticTileSurface &surface, const KiriView::DecodedTile &tile,
    const KiriView::ImageSurfaceDrawContext &context, bool resolutionIndependent)
{
    const QSize imageSize = surface.imageSize();
    if (imageSize.isEmpty() || tile.textureLevelRect.isEmpty() || tile.image.isNull()) {
        return std::nullopt;
    }

    QRectF sourceRect = resolutionIndependent ? tile.displaySourceRectF : QRectF();
    if (sourceRect.isEmpty()) {
        sourceRect = QRectF(tile.displaySourceRect);
    }
    if (sourceRect.isEmpty()) {
        sourceRect
            = QRectF(surface.pyramid().sourceRectForLevelRect(tile.key.level, tile.levelRect));
    }
    if (sourceRect.isEmpty()) {
        return std::nullopt;
    }

    const QRectF textureRect = tileTextureRect(tile);
    const QRectF tileTarget
        = tileTargetRect(sourceRect, imageSize, context.targetRect, context.rotationDegrees);
    if (!context.visibleItemRect.isEmpty() && !tileTarget.intersects(context.visibleItemRect)) {
        return std::nullopt;
    }

    return drawEntry(
        KiriView::ImageSurfaceDrawIdentity {
            KiriView::ImageSurfaceDrawIdentityKind::Tile,
            tile.key,
            resolutionIndependent,
        },
        tile.image, tileTarget, textureRect, context.rotationDegrees);
}

void appendDrawIdentities(KiriView::ImageRenderFrame *frame)
{
    frame->drawIdentities.reserve(frame->drawEntries.size());
    for (const KiriView::ImageSurfaceDrawEntry &entry : frame->drawEntries) {
        frame->drawIdentities.push_back(entry.identity);
    }
}

void appendRasterTileRequests(KiriView::ImageRenderFrame *frame,
    const KiriView::StaticTileSurface &surface, const KiriView::ImageRenderFrameInput &input)
{
    const KiriView::TileVisibilityContext visibilityContext {
        input.drawContext.displaySize,
        input.drawContext.visibleItemRect,
        input.drawContext.devicePixelRatio,
        input.drawContext.rotationDegrees,
    };
    for (const KiriView::TileKey &key :
        KiriView::visibleTileKeys(surface.pyramid(), visibilityContext)) {
        if (surface.containsTile(key) || input.tileDecodeExclusions.contains(key)) {
            continue;
        }

        KiriView::TileRequest request = surface.pyramid().requestForTile(key);
        if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()
            || request.displaySourceRect.isEmpty()) {
            continue;
        }

        frame->missingTileRequests.push_back(std::move(request));
    }
}

void appendSvgRasterBucketTileRequests(KiriView::ImageRenderFrame *frame,
    const KiriView::StaticTileSurface &surface, const KiriView::ImageRenderFrameInput &input)
{
    const QSizeF sourceDisplaySize = KiriView::rotatedImageSize(
        input.drawContext.displaySize, input.drawContext.rotationDegrees);
    const QRectF sourceVisibleItemRect = KiriView::unrotatedVisibleRectForRotation(
        sourceDisplaySize, input.drawContext.visibleItemRect, input.drawContext.rotationDegrees);
    for (KiriView::TileRequest request :
        KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(surface.imageSize(),
            surface.pyramid().tileSize(), surface.pyramid().apronSourcePixels(), sourceDisplaySize,
            sourceVisibleItemRect, input.drawContext.devicePixelRatio)) {
        if (surface.containsTile(request.key) || input.tileDecodeExclusions.contains(request.key)) {
            continue;
        }

        if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()
            || request.displaySourceRect.isEmpty()) {
            continue;
        }

        frame->missingTileRequests.push_back(std::move(request));
    }
}

void appendLegacyFrame(KiriView::ImageRenderFrame *frame,
    const KiriView::LegacyFrameSurface &surface, const KiriView::ImageRenderFrameInput &input)
{
    if (surface.image.isNull()) {
        return;
    }

    frame->drawEntries.push_back(fullImageDrawEntry(
        KiriView::ImageSurfaceDrawIdentity {
            KiriView::ImageSurfaceDrawIdentityKind::LegacyFrame,
            {},
            false,
        },
        surface.image, input.drawContext.targetRect, input.drawContext.rotationDegrees));
}

void appendStaticFrame(KiriView::ImageRenderFrame *frame,
    const KiriView::StaticTileSurface &surface, const KiriView::ImageRenderFrameInput &input)
{
    if (!surface.isValid()) {
        return;
    }

    frame->drawEntries.push_back(fullImageDrawEntry(
        KiriView::ImageSurfaceDrawIdentity {
            KiriView::ImageSurfaceDrawIdentityKind::Preview,
            {},
            false,
        },
        surface.preview(), input.drawContext.targetRect, input.drawContext.rotationDegrees));

    frame->tileRequestSource = surface.source();
    if (frame->tileRequestSource == nullptr) {
        return;
    }

    const bool resolutionIndependent = frame->tileRequestSource->isResolutionIndependent();
    frame->activeTileLayer = KiriView::activeTileLayer(surface.pyramid(),
        KiriView::TileVisibilityContext {
            input.drawContext.displaySize,
            input.drawContext.visibleItemRect,
            input.drawContext.devicePixelRatio,
            input.drawContext.rotationDegrees,
        },
        resolutionIndependent);

    for (const KiriView::DecodedTile &tile : surface.tiles()) {
        if (!frame->activeTileLayer.contains(tile.key)) {
            continue;
        }

        std::optional<KiriView::ImageSurfaceDrawEntry> entry
            = staticTileDrawEntry(surface, tile, input.drawContext, resolutionIndependent);
        if (entry.has_value()) {
            frame->drawEntries.push_back(std::move(*entry));
        }
    }

    const QSizeF sourceDisplaySize = KiriView::rotatedImageSize(
        input.drawContext.displaySize, input.drawContext.rotationDegrees);
    if (!resolutionIndependent
        && KiriView::tileFirstDisplayIsSufficient(surface.pyramid(), sourceDisplaySize,
            input.drawContext.devicePixelRatio,
            surface.displayHints().firstDisplayPixelsPerSourcePixel)) {
        frame->tileRequestSource.reset();
        return;
    }

    if (resolutionIndependent) {
        appendSvgRasterBucketTileRequests(frame, surface, input);
    } else {
        appendRasterTileRequests(frame, surface, input);
    }

    if (frame->missingTileRequests.empty()) {
        frame->tileRequestSource.reset();
    }
}
}

namespace KiriView {
ImageRenderFrame projectImageRenderFrame(const ImageRenderFrameInput &input)
{
    ImageRenderFrame frame;
    frame.surfaceIdentity = input.surface == nullptr ? 0 : input.surface->identity();
    frame.surfaceRevision = input.surfaceRevision;
    frame.drawContext = input.drawContext;
    frame.pageRole = input.pageRole;

    if (input.surface == nullptr || input.surface->isNull()
        || input.drawContext.targetRect.isEmpty()) {
        return frame;
    }

    frame.renderable = true;
    if (const auto *legacySurface = input.surface->legacyFrameSurface()) {
        appendLegacyFrame(&frame, *legacySurface, input);
    } else if (const auto *staticSurface = input.surface->staticTileSurface()) {
        appendStaticFrame(&frame, *staticSurface, input);
    }

    appendDrawIdentities(&frame);
    return frame;
}
}
