// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/rasterdisplaybucketpolicy.h"

#include "cache/imagebytecost.h"
#include "kiriview/src/policy/rasterdisplaybucketpolicy.cxx.h"
#include "rendering/imagerotation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

namespace KiriView {
namespace {
    constexpr qreal svgDisplayBucketScaleFactor = 1.5;

    int displayImageQualityIndex(DisplayImageQuality quality)
    {
        switch (quality) {
        case DisplayImageQuality::Exact:
            return 0;
        case DisplayImageQuality::FirstDisplay:
            return 1;
        case DisplayImageQuality::ThumbnailPreview:
            return 2;
        case DisplayImageQuality::BoundedDetail:
            return 3;
        case DisplayImageQuality::Unsupported:
            return 4;
        case DisplayImageQuality::Failed:
            return 5;
        }

        return 5;
    }

    DisplayImageQuality displayImageQualityFromIndex(int quality)
    {
        switch (quality) {
        case 0:
            return DisplayImageQuality::Exact;
        case 1:
            return DisplayImageQuality::FirstDisplay;
        case 2:
            return DisplayImageQuality::ThumbnailPreview;
        case 3:
            return DisplayImageQuality::BoundedDetail;
        case 4:
            return DisplayImageQuality::Unsupported;
        case 5:
            return DisplayImageQuality::Failed;
        }

        return DisplayImageQuality::Failed;
    }

    RasterDisplayBucketStatus bucketStatusFromRust(RustRasterDisplayBucketStatus status)
    {
        switch (status) {
        case RustRasterDisplayBucketStatus::Exact:
            return RasterDisplayBucketStatus::Exact;
        case RustRasterDisplayBucketStatus::FirstDisplaySufficient:
            return RasterDisplayBucketStatus::FirstDisplaySufficient;
        case RustRasterDisplayBucketStatus::RefinementNeeded:
            return RasterDisplayBucketStatus::RefinementNeeded;
        case RustRasterDisplayBucketStatus::BoundedDetail:
            return RasterDisplayBucketStatus::BoundedDetail;
        case RustRasterDisplayBucketStatus::UnsupportedTooLarge:
            return RasterDisplayBucketStatus::UnsupportedTooLarge;
        case RustRasterDisplayBucketStatus::Failed:
            return RasterDisplayBucketStatus::Failed;
        }

        return RasterDisplayBucketStatus::Failed;
    }

    RustRasterDisplayBucketInput rustBucketInput(const RasterDisplayBucketPolicyInput &input)
    {
        return RustRasterDisplayBucketInput {
            input.originalSize.width(),
            input.originalSize.height(),
            input.currentRasterSize.width(),
            input.currentRasterSize.height(),
            displayImageQualityIndex(input.currentQuality),
            input.displaySize.width(),
            input.displaySize.height(),
            input.visibleItemRect.x(),
            input.visibleItemRect.y(),
            input.visibleItemRect.width(),
            input.visibleItemRect.height(),
            input.devicePixelRatio,
            input.rotationDegrees,
            input.maximumTextureSize,
            static_cast<std::int64_t>(input.displayImageByteBudget),
        };
    }

    RasterDisplayBucketKey bucketKeyFromRust(const RustRasterDisplayBucketKey &key)
    {
        return RasterDisplayBucketKey {
            QSize(key.raster_width, key.raster_height),
            key.exact,
            key.maximum_texture_size,
            static_cast<qsizetype>(key.display_image_byte_budget),
        };
    }

    bool finitePositive(qreal value) { return std::isfinite(value) && value > 0.0; }

    bool svgDisplayDemandIsValid(const RasterDisplayBucketPolicyInput &input)
    {
        return input.originalSize.isValid() && !input.originalSize.isEmpty()
            && finitePositive(input.displaySize.width())
            && finitePositive(input.displaySize.height())
            && std::isfinite(input.visibleItemRect.x()) && std::isfinite(input.visibleItemRect.y())
            && finitePositive(input.visibleItemRect.width())
            && finitePositive(input.visibleItemRect.height())
            && finitePositive(input.devicePixelRatio);
    }

    qreal svgDesiredScale(const RasterDisplayBucketPolicyInput &input)
    {
        qreal displayWidth = input.displaySize.width();
        qreal displayHeight = input.displaySize.height();
        if (imageRotationSwapsAxes(input.rotationDegrees)) {
            std::swap(displayWidth, displayHeight);
        }

        const qreal physicalWidth = displayWidth * input.devicePixelRatio;
        const qreal physicalHeight = displayHeight * input.devicePixelRatio;
        if (!finitePositive(physicalWidth) || !finitePositive(physicalHeight)) {
            return 0.0;
        }

        return std::max(physicalWidth / input.originalSize.width(),
            physicalHeight / input.originalSize.height());
    }

    qreal svgBucketScale(qreal desiredScale)
    {
        if (!finitePositive(desiredScale)) {
            return 0.0;
        }

        qreal scale = 1.0;
        while (scale < desiredScale) {
            scale *= svgDisplayBucketScaleFactor;
            if (!finitePositive(scale)) {
                return 0.0;
            }
        }
        return scale;
    }

    int svgScaledDimension(int source, qreal scale)
    {
        if (source <= 0 || !finitePositive(scale)) {
            return 0;
        }

        const qreal dimension = std::ceil(static_cast<qreal>(source) * scale);
        if (!finitePositive(dimension)
            || dimension > static_cast<qreal>(std::numeric_limits<int>::max())) {
            return 0;
        }
        return std::max(1, static_cast<int>(dimension));
    }

    QSize svgBucketSize(const QSize &originalSize, qreal scale)
    {
        return QSize(svgScaledDimension(originalSize.width(), scale),
            svgScaledDimension(originalSize.height(), scale));
    }

    bool displayImageSizeIsSafe(const QSize &size, const RasterDisplayBucketPolicyInput &input)
    {
        if (size.isEmpty() || input.maximumTextureSize <= 0
            || size.width() > input.maximumTextureSize
            || size.height() > input.maximumTextureSize) {
            return false;
        }

        const qsizetype byteCost = estimatedRgbaByteCost(size);
        return byteCost > 0 && byteCost <= input.displayImageByteBudget;
    }

    bool rasterSizeSatisfies(const QSize &rasterSize, const QSize &bucketSize)
    {
        return !rasterSize.isEmpty() && !bucketSize.isEmpty()
            && rasterSize.width() >= bucketSize.width()
            && rasterSize.height() >= bucketSize.height();
    }

    RasterDisplayBucketKey displayBucketKey(
        const QSize &rasterSize, const RasterDisplayBucketPolicyInput &input, bool exact)
    {
        return RasterDisplayBucketKey {
            rasterSize,
            exact,
            input.maximumTextureSize,
            input.displayImageByteBudget,
        };
    }

    RasterDisplayBucketDecision displayBucketDecision(RasterDisplayBucketStatus status,
        DisplayImageQuality quality, RasterDisplayBucketKey bucketKey, bool refinementRequired,
        bool currentImageRetained)
    {
        return RasterDisplayBucketDecision {
            status,
            quality,
            std::move(bucketKey),
            refinementRequired,
            currentImageRetained,
        };
    }
}

RasterDisplayBucketDecision rasterDisplayBucketDecision(const RasterDisplayBucketPolicyInput &input)
{
    const RustRasterDisplayBucketDecision decision
        = rustRasterDisplayBucketDecision(rustBucketInput(input));
    return RasterDisplayBucketDecision {
        bucketStatusFromRust(decision.status),
        displayImageQualityFromIndex(decision.quality),
        bucketKeyFromRust(decision.bucket_key),
        decision.refinement_required,
        decision.current_image_retained,
    };
}

RasterDisplayBucketDecision svgDisplayBucketDecision(const RasterDisplayBucketPolicyInput &input)
{
    if (!svgDisplayDemandIsValid(input)) {
        return displayBucketDecision(RasterDisplayBucketStatus::Failed, DisplayImageQuality::Failed,
            displayBucketKey({}, input, false), false, false);
    }

    const qreal bucketScale = svgBucketScale(svgDesiredScale(input));
    const QSize bucketSize = svgBucketSize(input.originalSize, bucketScale);
    if (bucketSize.isEmpty()) {
        return displayBucketDecision(RasterDisplayBucketStatus::Failed, DisplayImageQuality::Failed,
            displayBucketKey({}, input, false), false, false);
    }

    const bool hasCurrent = !input.currentRasterSize.isEmpty();
    const bool currentIsSafe = hasCurrent && displayImageSizeIsSafe(input.currentRasterSize, input);
    if (displayImageSizeIsSafe(bucketSize, input)) {
        if (currentIsSafe && rasterSizeSatisfies(input.currentRasterSize, bucketSize)) {
            const bool currentIsExact = input.currentQuality == DisplayImageQuality::Exact;
            return displayBucketDecision(currentIsExact
                    ? RasterDisplayBucketStatus::Exact
                    : RasterDisplayBucketStatus::FirstDisplaySufficient,
                input.currentQuality,
                displayBucketKey(input.currentRasterSize, input, currentIsExact), false, true);
        }

        return displayBucketDecision(RasterDisplayBucketStatus::RefinementNeeded,
            DisplayImageQuality::Exact, displayBucketKey(bucketSize, input, true), true,
            hasCurrent);
    }

    if (currentIsSafe) {
        return displayBucketDecision(RasterDisplayBucketStatus::BoundedDetail,
            DisplayImageQuality::BoundedDetail,
            displayBucketKey(input.currentRasterSize, input, false), false, true);
    }

    return displayBucketDecision(RasterDisplayBucketStatus::UnsupportedTooLarge,
        DisplayImageQuality::Unsupported, displayBucketKey({}, input, false), false, false);
}
}
