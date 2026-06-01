// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailcachelookup.h"

#include "async/imageioworkerjob.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/thumbnailcache.cxx.h"

#include <QImage>
#include <cstdint>
#include <utility>

namespace {
KiriView::RustThumbnailCacheBucket rustBucket(
    KiriView::ActiveNavigationThumbnailDemandBucket bucket)
{
    switch (bucket) {
    case KiriView::ActiveNavigationThumbnailDemandBucket::None:
        return KiriView::RustThumbnailCacheBucket::None;
    case KiriView::ActiveNavigationThumbnailDemandBucket::Normal:
        return KiriView::RustThumbnailCacheBucket::Normal;
    case KiriView::ActiveNavigationThumbnailDemandBucket::Large:
        return KiriView::RustThumbnailCacheBucket::Large;
    case KiriView::ActiveNavigationThumbnailDemandBucket::XLarge:
        return KiriView::RustThumbnailCacheBucket::XLarge;
    case KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge:
        return KiriView::RustThumbnailCacheBucket::XxLarge;
    }

    return KiriView::RustThumbnailCacheBucket::None;
}

KiriView::ActiveNavigationThumbnailDemandBucket thumbnailBucket(
    KiriView::RustThumbnailCacheBucket bucket)
{
    switch (bucket) {
    case KiriView::RustThumbnailCacheBucket::None:
        return KiriView::ActiveNavigationThumbnailDemandBucket::None;
    case KiriView::RustThumbnailCacheBucket::Normal:
        return KiriView::ActiveNavigationThumbnailDemandBucket::Normal;
    case KiriView::RustThumbnailCacheBucket::Large:
        return KiriView::ActiveNavigationThumbnailDemandBucket::Large;
    case KiriView::RustThumbnailCacheBucket::XLarge:
        return KiriView::ActiveNavigationThumbnailDemandBucket::XLarge;
    case KiriView::RustThumbnailCacheBucket::XxLarge:
        return KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge;
    }

    return KiriView::ActiveNavigationThumbnailDemandBucket::None;
}

KiriView::ThumbnailCacheLookupStatus thumbnailStatus(
    KiriView::RustThumbnailCacheLookupStatus status)
{
    switch (status) {
    case KiriView::RustThumbnailCacheLookupStatus::Ready:
        return KiriView::ThumbnailCacheLookupStatus::Ready;
    case KiriView::RustThumbnailCacheLookupStatus::Missing:
        return KiriView::ThumbnailCacheLookupStatus::Missing;
    case KiriView::RustThumbnailCacheLookupStatus::Invalid:
        return KiriView::ThumbnailCacheLookupStatus::Invalid;
    case KiriView::RustThumbnailCacheLookupStatus::Failed:
        return KiriView::ThumbnailCacheLookupStatus::Failed;
    }

    return KiriView::ThumbnailCacheLookupStatus::Failed;
}

KiriView::ThumbnailCacheLookupResult lookupThumbnailCache(
    const KiriView::ThumbnailCacheLookupRequest &request)
{
    const KiriView::RustThumbnailCacheLookupResult rustResult
        = KiriView::rustLookupDisplayThumbnailRgba8(
            KiriView::Bridge::rustBytes(request.localPathBytes),
            rustBucket(request.requestedBucket));

    KiriView::ThumbnailCacheLookupResult result;
    result.status = thumbnailStatus(rustResult.status);
    result.requestedBucket = thumbnailBucket(rustResult.requested_bucket);
    result.sourceBucket = thumbnailBucket(rustResult.source_bucket);
    result.sourceCachePath = KiriView::Bridge::qtString(rustResult.source_cache_path);
    result.errorString = KiriView::Bridge::qtString(rustResult.error);

    if (result.status != KiriView::ThumbnailCacheLookupStatus::Ready || rustResult.width <= 0
        || rustResult.height <= 0 || rustResult.stride <= 0) {
        return result;
    }

    const QByteArray pixels = KiriView::Bridge::qtByteArray(rustResult.pixels);
    const QImage image(reinterpret_cast<const uchar *>(pixels.constData()), rustResult.width,
        rustResult.height, rustResult.stride, QImage::Format_RGBA8888);
    if (image.isNull()) {
        result.status = KiriView::ThumbnailCacheLookupStatus::Failed;
        result.errorString = QStringLiteral("thumbnail cache RGBA8 result could not form a QImage");
        return result;
    }

    result.image = image.copy();
    return result;
}
}

namespace KiriView {
ThumbnailCacheLookupProvider defaultThumbnailCacheLookupProvider()
{
    return [](QObject *receiver, ThumbnailCacheLookupRequest request,
               ThumbnailCacheLookupCallback callback) {
        return startImageIoWorkerJob(
            receiver, [request = std::move(request)]() { return lookupThumbnailCache(request); },
            [callback = std::move(callback)](ThumbnailCacheLookupResult result) mutable {
                if (callback) {
                    callback(std::move(result));
                }
            });
    };
}
}
