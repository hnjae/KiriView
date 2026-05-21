// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodeplan.h"

#include "imagetilegeometrybridge.h"
#include "imagetilevisibility.h"
#include "presentation/imagerotation.h"

#include <utility>

namespace KiriView {
namespace {
    void appendRasterTileRequests(ImageTileDecodePlan *plan, const StaticTileSurface &surface,
        const QSizeF &displaySize, const QRectF &visibleItemRect,
        const ImageDocumentRenderContext &context, int rotationDegrees,
        const ImageTileDecodeExclusions &exclusions)
    {
        const TileVisibilityContext visibilityContext {
            displaySize,
            visibleItemRect,
            context.devicePixelRatio,
            rotationDegrees,
        };
        for (const TileKey &key : visibleTileKeys(surface.pyramid(), visibilityContext)) {
            if (surface.containsTile(key) || exclusions.contains(key)) {
                continue;
            }

            TileRequest request = surface.pyramid().requestForTile(key);
            if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()
                || request.displaySourceRect.isEmpty()) {
                continue;
            }

            plan->requests.push_back(std::move(request));
        }
    }

    void appendSvgRasterBucketTileRequests(ImageTileDecodePlan *plan,
        const StaticTileSurface &surface, const QSizeF &displaySize, const QRectF &visibleItemRect,
        const ImageDocumentRenderContext &context, int rotationDegrees,
        const ImageTileDecodeExclusions &exclusions)
    {
        const QSizeF sourceDisplaySize = rotatedImageSize(displaySize, rotationDegrees);
        const QRectF sourceVisibleItemRect
            = unrotatedVisibleRectForRotation(sourceDisplaySize, visibleItemRect, rotationDegrees);
        for (TileRequest request :
            ImageTileGeometryBridge::svgRasterTileRequests(surface.imageSize(),
                surface.pyramid().tileSize(), surface.pyramid().apronSourcePixels(),
                sourceDisplaySize, sourceVisibleItemRect, context.devicePixelRatio)) {
            if (surface.containsTile(request.key) || exclusions.contains(request.key)) {
                continue;
            }

            if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()
                || request.displaySourceRect.isEmpty()) {
                continue;
            }

            plan->requests.push_back(std::move(request));
        }
    }
}

ImageTileDecodePlan imageTileDecodePlan(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees,
    const ImageTileDecodeExclusions &exclusions)
{
    if (displayedSurface == nullptr) {
        return {};
    }

    const StaticTileSurface *surface = displayedSurface->staticTileSurface();
    if (surface == nullptr || !surface->isValid()) {
        return {};
    }

    ImageTileDecodePlan plan;
    plan.source = surface->source();
    if (plan.source == nullptr) {
        return {};
    }

    const QSizeF sourceDisplaySize = rotatedImageSize(displaySize, rotationDegrees);
    if (!plan.source->isResolutionIndependent()
        && tileFirstDisplayIsSufficient(surface->pyramid(), sourceDisplaySize,
            context.devicePixelRatio, surface->displayHints().firstDisplayPixelsPerSourcePixel)) {
        return {};
    }

    if (plan.source->isResolutionIndependent()) {
        appendSvgRasterBucketTileRequests(
            &plan, *surface, displaySize, visibleItemRect, context, rotationDegrees, exclusions);
    } else {
        appendRasterTileRequests(
            &plan, *surface, displaySize, visibleItemRect, context, rotationDegrees, exclusions);
    }

    if (plan.requests.empty()) {
        plan.source.reset();
    }
    return plan;
}

ImageTileDecodeSchedulePlan imageTileDecodeSchedulePlan(ImageTileDecodeState &state,
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees)
{
    const ImageTileDecodeScheduleState schedule = state.beginSchedule(displayedSurface);
    ImageTileDecodePlan candidatePlan = imageTileDecodePlan(displayedSurface, displaySize,
        visibleItemRect, context, rotationDegrees, schedule.exclusions);
    if (candidatePlan.isEmpty()) {
        return {};
    }

    std::vector<TileRequest> requests
        = state.commitScheduleRequests(schedule, std::move(candidatePlan.requests));
    if (requests.empty()) {
        return {};
    }

    return ImageTileDecodeSchedulePlan {
        schedule.generation,
        std::move(candidatePlan.source),
        std::move(requests),
    };
}
}
