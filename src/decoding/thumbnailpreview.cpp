// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailpreview.h"

#include <QFile>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;

bool validImageSize(const QSize &size)
{
    return size.isValid() && !size.isEmpty() && size.width() > 0 && size.height() > 0;
}

bool supportedPreviewBucket(Bucket bucket)
{
    switch (bucket) {
    case Bucket::Large:
    case Bucket::XLarge:
    case Bucket::XXLarge:
        return true;
    case Bucket::None:
    case Bucket::Normal:
        break;
    }

    return false;
}

bool thumbnailFitsOriginal(const QSize &thumbnailSize, const QSize &originalSize)
{
    return thumbnailSize.width() <= originalSize.width()
        && thumbnailSize.height() <= originalSize.height();
}

bool aspectCompatible(const QSize &thumbnailSize, const QSize &originalSize)
{
    if (!validImageSize(thumbnailSize) || !validImageSize(originalSize)) {
        return false;
    }

    const long double left
        = static_cast<long double>(thumbnailSize.width()) * originalSize.height();
    const long double right
        = static_cast<long double>(thumbnailSize.height()) * originalSize.width();
    const long double maximum = std::max(std::fabsl(left), std::fabsl(right));
    if (maximum <= 0.0L) {
        return false;
    }

    return std::fabsl(left - right) / maximum <= 0.01L;
}

KiriView::XdgThumbnailPreviewResult baseResult(const KiriView::XdgThumbnailPreviewRequest &request,
    const KiriView::ThumbnailCacheLookupResult &lookupResult)
{
    return KiriView::XdgThumbnailPreviewResult {
        lookupResult.status,
        {},
        request.trustedOriginalSize,
        KiriView::DisplayImageQuality::ThumbnailPreview,
        lookupResult.requestedBucket,
        lookupResult.sourceBucket,
        lookupResult.sourceCachePath,
        lookupResult.errorString,
    };
}

KiriView::XdgThumbnailPreviewResult invalidResult(
    const KiriView::XdgThumbnailPreviewRequest &request,
    const KiriView::ThumbnailCacheLookupResult &lookupResult, QString errorString)
{
    KiriView::XdgThumbnailPreviewResult result = baseResult(request, lookupResult);
    result.status = KiriView::ThumbnailCacheLookupStatus::Invalid;
    result.errorString = std::move(errorString);
    return result;
}
}

namespace KiriView {
std::optional<ThumbnailCacheLookupRequest> xdgThumbnailPreviewCacheLookupRequest(
    const XdgThumbnailPreviewRequest &request)
{
    if (!request.stillImage || !request.sourceUrl.isLocalFile()
        || !validImageSize(request.trustedOriginalSize)
        || !supportedPreviewBucket(request.requestedBucket)) {
        return std::nullopt;
    }

    const QByteArray localPathBytes = QFile::encodeName(request.sourceUrl.toLocalFile());
    if (localPathBytes.isEmpty()) {
        return std::nullopt;
    }

    return ThumbnailCacheLookupRequest {
        localPathBytes,
        ThumbnailOriginalIdentity::fromLocalPathBytes(localPathBytes),
        request.requestedBucket,
    };
}

XdgThumbnailPreviewResult xdgThumbnailPreviewResult(
    const XdgThumbnailPreviewRequest &request, ThumbnailCacheLookupResult lookupResult)
{
    XdgThumbnailPreviewResult result = baseResult(request, lookupResult);
    if (lookupResult.status != ThumbnailCacheLookupStatus::Ready) {
        return result;
    }

    if (!validImageSize(request.trustedOriginalSize)) {
        return invalidResult(
            request, lookupResult, QStringLiteral("trusted original image size is unavailable"));
    }
    if (lookupResult.image.isNull()) {
        return invalidResult(
            request, lookupResult, QStringLiteral("thumbnail cache hit returned no image"));
    }
    if (!supportedPreviewBucket(lookupResult.requestedBucket)
        || !supportedPreviewBucket(lookupResult.sourceBucket)) {
        return invalidResult(
            request, lookupResult, QStringLiteral("thumbnail cache hit used an unsupported size"));
    }
    if (!thumbnailFitsOriginal(lookupResult.image.size(), request.trustedOriginalSize)) {
        return invalidResult(
            request, lookupResult, QStringLiteral("thumbnail image is larger than source image"));
    }
    if (!aspectCompatible(lookupResult.image.size(), request.trustedOriginalSize)) {
        return invalidResult(request, lookupResult,
            QStringLiteral("thumbnail aspect ratio does not match trusted source dimensions"));
    }

    result.image = std::move(lookupResult.image);
    return result;
}
}
