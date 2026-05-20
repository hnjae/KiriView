// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodeplan.h"

#include "imagetilegeometrybridge.h"
#include "imagetilevisibility.h"
#include "presentation/imagerotation.h"

#include <QPointF>
#include <QRect>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace KiriView {
namespace {
    constexpr qreal svgRasterScaleBucketFactor = 1.5;
    constexpr int svgRasterTileLevel = 0;

    int svgRasterScaleBucket(qreal requiredScale)
    {
        if (!std::isfinite(requiredScale) || requiredScale <= 0.0) {
            return 0;
        }

        int bucket = 0;
        qreal bucketScale = 1.0;
        if (requiredScale >= 1.0) {
            while (bucketScale <= requiredScale) {
                bucketScale *= svgRasterScaleBucketFactor;
                ++bucket;
            }
            return bucket;
        }

        while ((bucketScale / svgRasterScaleBucketFactor) > requiredScale) {
            bucketScale /= svgRasterScaleBucketFactor;
            --bucket;
        }
        return bucket;
    }

    qreal svgRasterScaleForBucket(int bucket)
    {
        return std::pow(svgRasterScaleBucketFactor, bucket);
    }

    QSize svgRasterSizeForBucket(const QSize &imageSize, int bucket)
    {
        const qreal scale = svgRasterScaleForBucket(bucket);
        if (imageSize.isEmpty() || !std::isfinite(scale) || scale <= 0.0) {
            return {};
        }

        const auto scaledDimension = [scale](int dimension) -> int {
            const qreal scaled = std::ceil(static_cast<qreal>(dimension) * scale);
            if (!std::isfinite(scaled) || scaled <= 0.0
                || scaled > static_cast<qreal>(std::numeric_limits<int>::max())) {
                return 0;
            }
            return static_cast<int>(scaled);
        };

        return QSize(scaledDimension(imageSize.width()), scaledDimension(imageSize.height()));
    }

    QRectF boundedPrefetchItemRect(const QSizeF &displaySize, const QRectF &visibleItemRect)
    {
        const QRectF imageRect(QPointF(0.0, 0.0), displaySize);
        return visibleItemRect
            .adjusted(-visibleItemRect.width(), -visibleItemRect.height(), visibleItemRect.width(),
                visibleItemRect.height())
            .intersected(imageRect);
    }

    void appendMissingTileKeys(std::vector<TileKey> *keys, const std::vector<TileKey> &candidates)
    {
        for (const TileKey &key : candidates) {
            if (std::find(keys->cbegin(), keys->cend(), key) == keys->cend()) {
                keys->push_back(key);
            }
        }
    }

    std::vector<TileKey> svgRasterBucketTileKeys(const TilePyramid &bucketPyramid,
        const QSizeF &displaySize, const QRectF &visibleItemRect, int rotationDegrees,
        int scaleBucket)
    {
        const QSizeF sourceDisplaySize = rotatedImageSize(displaySize, rotationDegrees);
        const QRectF sourceVisibleItemRect
            = unrotatedVisibleRectForRotation(sourceDisplaySize, visibleItemRect, rotationDegrees);

        const QRect visibleLevelRect = tileLevelRectForItemRect(
            bucketPyramid, svgRasterTileLevel, sourceDisplaySize, sourceVisibleItemRect);
        std::vector<TileKey> keys
            = bucketPyramid.tilesIntersectingLevelRect(svgRasterTileLevel, visibleLevelRect);

        const QRectF prefetchItemRect
            = boundedPrefetchItemRect(sourceDisplaySize, sourceVisibleItemRect);
        const QRect prefetchLevelRect = tileLevelRectForItemRect(
            bucketPyramid, svgRasterTileLevel, sourceDisplaySize, prefetchItemRect);
        appendMissingTileKeys(
            &keys, bucketPyramid.tilesIntersectingLevelRect(svgRasterTileLevel, prefetchLevelRect));

        for (TileKey &key : keys) {
            key.scaleBucket = scaleBucket;
        }
        return keys;
    }

    QRect intrinsicRectForBucketRect(
        const QRect &bucketRect, const QSize &bucketSize, const QSize &imageSize)
    {
        return ImageTileGeometryBridge::scaledIntegerRect(
            QRectF(bucketRect), QSizeF(bucketSize), imageSize);
    }

    TileRequest svgRasterBucketRequestForTile(
        const TilePyramid &bucketPyramid, const QSize &imageSize, const TileKey &key)
    {
        TileRequest request = bucketPyramid.requestForTile(key);
        request.sourceRect = intrinsicRectForBucketRect(
            request.textureLevelRect, bucketPyramid.imageSize(), imageSize);
        request.displaySourceRect
            = intrinsicRectForBucketRect(request.levelRect, bucketPyramid.imageSize(), imageSize);
        return request;
    }

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
        const qreal requiredScale = tileDisplayPixelsPerSourcePixel(
            surface.pyramid(), sourceDisplaySize, context.devicePixelRatio);
        if (!std::isfinite(requiredScale) || requiredScale <= 0.0) {
            return;
        }

        const int scaleBucket = svgRasterScaleBucket(requiredScale);
        const QSize bucketSize = svgRasterSizeForBucket(surface.imageSize(), scaleBucket);
        if (bucketSize.isEmpty()) {
            return;
        }

        const TilePyramid bucketPyramid(bucketSize);
        for (const TileKey &key : svgRasterBucketTileKeys(
                 bucketPyramid, displaySize, visibleItemRect, rotationDegrees, scaleBucket)) {
            if (surface.containsTile(key) || exclusions.contains(key)) {
                continue;
            }

            TileRequest request
                = svgRasterBucketRequestForTile(bucketPyramid, surface.imageSize(), key);
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
}
