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
kiriview::RustThumbnailCacheBucket rustBucket(
    kiriview::ActiveNavigationThumbnailDemandBucket bucket)
{
    switch (bucket) {
    case kiriview::ActiveNavigationThumbnailDemandBucket::None:
        return kiriview::RustThumbnailCacheBucket::None;
    case kiriview::ActiveNavigationThumbnailDemandBucket::Normal:
        return kiriview::RustThumbnailCacheBucket::Normal;
    case kiriview::ActiveNavigationThumbnailDemandBucket::Large:
        return kiriview::RustThumbnailCacheBucket::Large;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XLarge:
        return kiriview::RustThumbnailCacheBucket::XLarge;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge:
        return kiriview::RustThumbnailCacheBucket::XxLarge;
    }

    return kiriview::RustThumbnailCacheBucket::None;
}

kiriview::ActiveNavigationThumbnailDemandBucket thumbnailBucket(
    kiriview::RustThumbnailCacheBucket bucket)
{
    switch (bucket) {
    case kiriview::RustThumbnailCacheBucket::None:
        return kiriview::ActiveNavigationThumbnailDemandBucket::None;
    case kiriview::RustThumbnailCacheBucket::Normal:
        return kiriview::ActiveNavigationThumbnailDemandBucket::Normal;
    case kiriview::RustThumbnailCacheBucket::Large:
        return kiriview::ActiveNavigationThumbnailDemandBucket::Large;
    case kiriview::RustThumbnailCacheBucket::XLarge:
        return kiriview::ActiveNavigationThumbnailDemandBucket::XLarge;
    case kiriview::RustThumbnailCacheBucket::XxLarge:
        return kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge;
    }

    return kiriview::ActiveNavigationThumbnailDemandBucket::None;
}

kiriview::ThumbnailCacheLookupStatus thumbnailStatus(
    kiriview::RustThumbnailCacheLookupStatus status)
{
    switch (status) {
    case kiriview::RustThumbnailCacheLookupStatus::Ready:
        return kiriview::ThumbnailCacheLookupStatus::Ready;
    case kiriview::RustThumbnailCacheLookupStatus::Missing:
        return kiriview::ThumbnailCacheLookupStatus::Missing;
    case kiriview::RustThumbnailCacheLookupStatus::Invalid:
        return kiriview::ThumbnailCacheLookupStatus::Invalid;
    case kiriview::RustThumbnailCacheLookupStatus::Failed:
        return kiriview::ThumbnailCacheLookupStatus::Failed;
    }

    return kiriview::ThumbnailCacheLookupStatus::Failed;
}

kiriview::ThumbnailCacheLookupResult lookupThumbnailCache(
    const kiriview::ThumbnailCacheLookupRequest& request)
{
    kiriview::RustThumbnailCacheLookupResult rustResult;
    if (request.originalIdentity.isNonFileUri()) {
        const QByteArray uri = request.originalIdentity.uri.toUtf8();
        const QByteArray mimeType = request.originalIdentity.mimeType.toUtf8();
        rustResult
            = kiriview::rustLookupDisplayThumbnailNonFileUriRgba8(kiriview::Bridge::rustStr(uri),
                request.originalIdentity.mtimeSeconds, request.originalIdentity.originalByteSize,
                kiriview::Bridge::rustStr(mimeType), rustBucket(request.requestedBucket));
    } else {
        const QByteArray localPathBytes = request.originalIdentity.localPathBytes.isEmpty()
            ? request.localPathBytes
            : request.originalIdentity.localPathBytes;
        rustResult = kiriview::rustLookupDisplayThumbnailRgba8(
            kiriview::Bridge::rustBytes(localPathBytes), rustBucket(request.requestedBucket));
    }

    kiriview::ThumbnailCacheLookupResult result;
    result.status = thumbnailStatus(rustResult.status);
    result.requestedBucket = thumbnailBucket(rustResult.requested_bucket);
    result.sourceBucket = thumbnailBucket(rustResult.source_bucket);
    result.sourceCachePath = kiriview::Bridge::qtString(rustResult.source_cache_path);
    result.errorString = kiriview::Bridge::qtString(rustResult.error);

    if (result.status != kiriview::ThumbnailCacheLookupStatus::Ready || rustResult.width <= 0
        || rustResult.height <= 0 || rustResult.stride <= 0) {
        return result;
    }

    const QByteArray pixels = kiriview::Bridge::qtByteArray(rustResult.pixels);
    const QImage image(reinterpret_cast<const uchar*>(pixels.constData()), rustResult.width,
        rustResult.height, rustResult.stride, QImage::Format_RGBA8888);
    if (image.isNull()) {
        result.status = kiriview::ThumbnailCacheLookupStatus::Failed;
        result.errorString = QStringLiteral("thumbnail cache RGBA8 result could not form a QImage");
        return result;
    }

    result.image = image.copy();
    return result;
}
}

namespace kiriview {
ThumbnailCacheLookupProvider defaultThumbnailCacheLookupProvider(
    ImageWorkerScheduler workerScheduler)
{
    return [workerScheduler = std::move(workerScheduler)](QObject* receiver,
               ThumbnailCacheLookupRequest request, ThumbnailCacheLookupCallback callback) {
        return startImageIoWorkerJob(
            receiver, workerScheduler,
            [request = std::move(request)]() { return lookupThumbnailCache(request); },
            [callback = std::move(callback)](ThumbnailCacheLookupResult result) mutable {
                if (callback) {
                    callback(std::move(result));
                }
            });
    };
}
}
