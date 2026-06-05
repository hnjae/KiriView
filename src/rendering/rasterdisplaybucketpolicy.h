// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_RASTERDISPLAYBUCKETPOLICY_H
#define KIRIVIEW_RASTERDISPLAYBUCKETPOLICY_H

#include "rendering/displayimagequality.h"
#include "rendering/imagerendercontext.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QtGlobal>

namespace KiriView {
enum class RasterDisplayBucketStatus {
    Exact,
    FirstDisplaySufficient,
    RefinementNeeded,
    BoundedDetail,
    UnsupportedTooLarge,
    Failed,
};

struct RasterDisplayBucketKey {
    QSize rasterSize;
    bool exact = false;
    int maximumTextureSize = 0;
    qsizetype displayImageByteBudget = 0;

    friend bool operator==(const RasterDisplayBucketKey &left, const RasterDisplayBucketKey &right)
    {
        return left.rasterSize == right.rasterSize && left.exact == right.exact
            && left.maximumTextureSize == right.maximumTextureSize
            && left.displayImageByteBudget == right.displayImageByteBudget;
    }

    friend bool operator!=(const RasterDisplayBucketKey &left, const RasterDisplayBucketKey &right)
    {
        return !(left == right);
    }
};

struct RasterDisplayBucketPolicyInput {
    QSize originalSize;
    QSize currentRasterSize;
    DisplayImageQuality currentQuality = DisplayImageQuality::FirstDisplay;
    QSizeF displaySize;
    QRectF visibleItemRect;
    qreal devicePixelRatio = 1.0;
    int rotationDegrees = 0;
    int maximumTextureSize = 0;
    qsizetype displayImageByteBudget = 0;
};

struct RasterDisplayBucketDecision {
    RasterDisplayBucketStatus status = RasterDisplayBucketStatus::Failed;
    DisplayImageQuality quality = DisplayImageQuality::Failed;
    RasterDisplayBucketKey bucketKey;
    bool refinementRequired = false;
    bool currentImageRetained = false;
};

struct RasterDisplayRefinementDemandKey {
    QString sourceIdentity;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    quint64 displaySourceRevision = 0;
    quint64 zoomGeneration = 0;
    quint64 devicePixelRatioGeneration = 0;
    quint64 renderContextGeneration = 0;
    quint64 allocationGeneration = 0;
    quint64 rotationGeneration = 0;
    RasterDisplayBucketKey bucketKey;

    friend bool operator==(
        const RasterDisplayRefinementDemandKey &left, const RasterDisplayRefinementDemandKey &right)
    {
        return left.sourceIdentity == right.sourceIdentity && left.pageRole == right.pageRole
            && left.displaySourceRevision == right.displaySourceRevision
            && left.zoomGeneration == right.zoomGeneration
            && left.devicePixelRatioGeneration == right.devicePixelRatioGeneration
            && left.renderContextGeneration == right.renderContextGeneration
            && left.allocationGeneration == right.allocationGeneration
            && left.rotationGeneration == right.rotationGeneration
            && left.bucketKey == right.bucketKey;
    }

    friend bool operator!=(
        const RasterDisplayRefinementDemandKey &left, const RasterDisplayRefinementDemandKey &right)
    {
        return !(left == right);
    }
};

RasterDisplayBucketDecision rasterDisplayBucketDecision(
    const RasterDisplayBucketPolicyInput &input);
}

#endif
