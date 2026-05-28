// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilerequestplan.h"

#include "imagesurface.h"
#include "imagetilegeometrypolicy.h"
#include "imagetilevisibility.h"

#include <utility>

namespace {
KiriView::TileVisibilityContext tileVisibilityContext(
    const KiriView::ImageSurfaceDrawContext &drawContext)
{
    return KiriView::TileVisibilityContext {
        drawContext.displaySize,
        drawContext.visibleItemRect,
        drawContext.devicePixelRatio,
        drawContext.rotationDegrees,
    };
}

bool tileRequestCanBeScheduled(const KiriView::TileRequest &request)
{
    return !request.textureLevelRect.isEmpty() && !request.sourceRect.isEmpty()
        && !request.displaySourceRect.isEmpty();
}

void appendRequestIfMissing(KiriView::ImageTileRequestPlan *plan,
    const KiriView::StaticTileSurface &surface,
    const KiriView::ImageTileDecodeExclusions &exclusions, KiriView::TileRequest request)
{
    if (surface.containsTile(request.key) || exclusions.contains(request.key)
        || !tileRequestCanBeScheduled(request)) {
        return;
    }

    plan->missingRequests.push_back(std::move(request));
}

void appendRasterTileRequests(KiriView::ImageTileRequestPlan *plan,
    const KiriView::StaticTileSurface &surface, const KiriView::TileVisibilityContext &context,
    const KiriView::ImageTileDecodeExclusions &exclusions)
{
    for (const KiriView::TileKey &key : KiriView::visibleTileKeys(surface.pyramid(), context)) {
        appendRequestIfMissing(plan, surface, exclusions, surface.pyramid().requestForTile(key));
    }
}

void appendResolutionIndependentTileRequests(KiriView::ImageTileRequestPlan *plan,
    const KiriView::StaticTileSurface &surface, const KiriView::TileVisibilityContext &context,
    const KiriView::ImageTileDecodeExclusions &exclusions)
{
    const KiriView::TileSourceVisibilityContext sourceContext
        = KiriView::tileSourceVisibilityContext(context);
    for (KiriView::TileRequest request :
        KiriView::ImageTileGeometryPolicy::svgRasterTileRequests(surface.imageSize(),
            surface.pyramid().tileSize(), surface.pyramid().apronSourcePixels(),
            sourceContext.displaySize, sourceContext.visibleItemRect, context.devicePixelRatio)) {
        appendRequestIfMissing(plan, surface, exclusions, std::move(request));
    }
}

bool rasterFirstDisplaySuppressesRequests(
    const KiriView::StaticTileSurface &surface, const KiriView::TileVisibilityContext &context)
{
    const KiriView::TileSourceVisibilityContext sourceContext
        = KiriView::tileSourceVisibilityContext(context);
    return KiriView::tileFirstDisplayIsSufficient(surface.pyramid(), sourceContext.displaySize,
        context.devicePixelRatio, surface.displayHints().firstDisplayPixelsPerSourcePixel);
}
}

namespace KiriView {
ImageTileRequestPlan planImageTileRequests(const ImageTileRequestPlanInput &input)
{
    ImageTileRequestPlan plan;
    if (input.surface == nullptr || !input.surface->isValid()) {
        return plan;
    }

    std::shared_ptr<ImageTileSource> source = input.surface->source();
    if (source == nullptr) {
        return plan;
    }

    const TileVisibilityContext context = tileVisibilityContext(input.drawContext);
    plan.activeTileLayer
        = activeTileLayer(input.surface->pyramid(), context, input.resolutionIndependent);

    if (!input.resolutionIndependent
        && rasterFirstDisplaySuppressesRequests(*input.surface, context)) {
        return plan;
    }

    if (input.resolutionIndependent) {
        appendResolutionIndependentTileRequests(
            &plan, *input.surface, context, input.tileDecodeExclusions);
    } else {
        appendRasterTileRequests(&plan, *input.surface, context, input.tileDecodeExclusions);
    }

    if (!plan.missingRequests.empty()) {
        plan.source = std::move(source);
    }
    return plan;
}
}
