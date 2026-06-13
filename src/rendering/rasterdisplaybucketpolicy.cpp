// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/rasterdisplaybucketpolicy.h"

#include "kiriview/src/policy/rasterdisplaybucketpolicy.cxx.h"

#include <cstdint>

namespace kiriview {
namespace {
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

    RasterDisplayBucketDecision bucketDecisionFromRust(
        const RustRasterDisplayBucketDecision &decision)
    {
        return RasterDisplayBucketDecision {
            bucketStatusFromRust(decision.status),
            displayImageQualityFromIndex(decision.quality),
            bucketKeyFromRust(decision.bucket_key),
            decision.refinement_required,
            decision.current_image_retained,
        };
    }
}

RasterDisplayBucketDecision rasterDisplayBucketDecision(const RasterDisplayBucketPolicyInput &input)
{
    return bucketDecisionFromRust(rustRasterDisplayBucketDecision(rustBucketInput(input)));
}

RasterDisplayBucketDecision svgDisplayBucketDecision(const RasterDisplayBucketPolicyInput &input)
{
    return bucketDecisionFromRust(rustSvgDisplayBucketDecision(rustBucketInput(input)));
}
}
