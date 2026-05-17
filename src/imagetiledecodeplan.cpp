// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodeplan.h"

#include "imagerotation.h"
#include "imagetilevisibility.h"

#include <utility>

namespace KiriView {
ImageTileDecodePlan imageTileDecodePlan(
    const std::shared_ptr<DisplayedImageSurface> &displayedSurface, const QSizeF &displaySize,
    const QRectF &visibleItemRect, const ImageDocumentRenderContext &context, int rotationDegrees,
    const QSet<TileKey> &pendingTileKeys, const QSet<TileKey> &failedTileKeys)
{
    if (displayedSurface == nullptr) {
        return {};
    }

    const StaticTileSurface *surface = displayedSurface->staticTileSurface();
    if (surface == nullptr || !surface->isValid()) {
        return {};
    }

    const QSizeF sourceDisplaySize = rotatedImageSize(displaySize, rotationDegrees);
    if (tileFirstDisplayIsSufficient(surface->pyramid(), sourceDisplaySize,
            context.devicePixelRatio, surface->displayHints().firstDisplayPixelsPerSourcePixel)) {
        return {};
    }

    ImageTileDecodePlan plan;
    plan.source = surface->source();
    if (plan.source == nullptr) {
        return {};
    }

    const TileVisibilityContext visibilityContext {
        displaySize,
        visibleItemRect,
        context.devicePixelRatio,
        rotationDegrees,
    };
    for (const TileKey &key : visibleTileKeys(surface->pyramid(), visibilityContext)) {
        if (surface->containsTile(key) || pendingTileKeys.contains(key)
            || failedTileKeys.contains(key)) {
            continue;
        }

        TileRequest request = surface->pyramid().requestForTile(key);
        if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()) {
            continue;
        }

        plan.requests.push_back(std::move(request));
    }

    if (plan.requests.empty()) {
        plan.source.reset();
    }
    return plan;
}
}
