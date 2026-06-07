// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailgeneration.h"

#include "archive/mediaentrysourcebackend.h"
#include "async/imageioworkerjob.h"
#include "bridge/rustqtconversion.h"
#include "decoding/decodedimageresult.h"
#include "decoding/kiriimagedecoder.h"
#include "kiriview/src/policy/thumbnailcache.cxx.h"
#include "rendering/staticimage.h"
#include "thumbnail/thumbnailcachelookup.h"

#include <QFile>
#include <QImage>
#include <QSize>
#include <Qt>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

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

int bucketMaxEdge(KiriView::ActiveNavigationThumbnailDemandBucket bucket)
{
    switch (bucket) {
    case KiriView::ActiveNavigationThumbnailDemandBucket::Normal:
        return 128;
    case KiriView::ActiveNavigationThumbnailDemandBucket::Large:
        return 256;
    case KiriView::ActiveNavigationThumbnailDemandBucket::XLarge:
        return 512;
    case KiriView::ActiveNavigationThumbnailDemandBucket::XXLarge:
        return 1024;
    case KiriView::ActiveNavigationThumbnailDemandBucket::None:
        break;
    }

    return 0;
}

KiriView::ThumbnailGenerationResult failedResult(
    KiriView::ActiveNavigationThumbnailDemandBucket bucket, QString errorString)
{
    return KiriView::ThumbnailGenerationResult {
        KiriView::ThumbnailGenerationStatus::Failed,
        {},
        bucket,
        {},
        std::move(errorString),
    };
}

KiriView::ThumbnailCacheLookupStatus thumbnailCacheStatus(
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

KiriView::ThumbnailCacheLookupResult lookupResultFromRust(
    const KiriView::RustThumbnailCacheLookupResult &rustResult)
{
    KiriView::ThumbnailCacheLookupResult result;
    result.status = thumbnailCacheStatus(rustResult.status);
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

KiriView::ThumbnailGenerationResult readyResultFromCache(
    const KiriView::ThumbnailCacheLookupResult &lookup)
{
    return KiriView::ThumbnailGenerationResult {
        KiriView::ThumbnailGenerationStatus::Ready,
        lookup.image,
        lookup.requestedBucket,
        lookup.sourceCachePath,
        {},
    };
}

QSize boundedSize(const QSize &size, int maximumLongEdge)
{
    if (size.isEmpty() || maximumLongEdge <= 0) {
        return {};
    }

    const int longEdge = std::max(size.width(), size.height());
    if (longEdge <= maximumLongEdge) {
        return size;
    }

    return size.scaled(QSize(maximumLongEdge, maximumLongEdge), Qt::KeepAspectRatio);
}

QImage thumbnailFrame(QImage image, int maximumLongEdge)
{
    const QSize targetSize = boundedSize(image.size(), maximumLongEdge);
    if (targetSize.isEmpty()) {
        return {};
    }
    if (targetSize == image.size()) {
        return image;
    }

    return image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage renderedThumbnailImage(
    const KiriView::DecodedImage &decoded, int maximumLongEdge, QString *errorString)
{
    return std::visit(
        [maximumLongEdge, errorString](const auto &image) -> QImage {
            using Image = std::decay_t<decltype(image)>;
            if constexpr (std::is_same_v<Image, KiriView::StaticDecodedImage>) {
                if (image.displayImage.refinementSource == nullptr) {
                    if (errorString != nullptr) {
                        *errorString = QStringLiteral("static image source is unavailable");
                    }
                    return {};
                }
                return image.displayImage.refinementSource->decodeBlockingDisplayImage(
                    maximumLongEdge, errorString);
            } else {
                Q_UNUSED(errorString)
                return thumbnailFrame(image.firstFrame, maximumLongEdge);
            }
        },
        decoded);
}

KiriView::MediaEntrySourceImageDataResult loadOpenedCollectionThumbnailBytes(
    const KiriView::ThumbnailGenerationRequest &request)
{
    return KiriView::loadMediaEntrySourceImageData(
        request.openedCollectionScope, request.sourceUrl);
}

KiriView::MediaEntrySourceThumbnailMetadataResult loadOpenedCollectionThumbnailMetadata(
    const KiriView::ThumbnailGenerationRequest &request)
{
    return KiriView::loadMediaEntrySourceThumbnailMetadata(
        request.openedCollectionScope, request.sourceUrl);
}

std::optional<KiriView::ThumbnailOriginalIdentity> openedCollectionVirtualOriginalIdentity(
    const KiriView::MediaEntrySourceThumbnailMetadata &metadata)
{
    if (metadata.checksum.algorithm != KiriView::MediaEntryContentChecksumAlgorithm::Crc32
        || metadata.checksum.value > std::numeric_limits<std::uint32_t>::max()
        || metadata.uncompressedSize < 0) {
        return std::nullopt;
    }

    const QString uri
        = KiriView::Bridge::qtString(KiriView::rustVirtualThumbnailArchiveEntryCrc32Uri(
            static_cast<std::uint32_t>(metadata.checksum.value),
            static_cast<std::uint64_t>(metadata.uncompressedSize)));
    if (uri.isEmpty()) {
        return std::nullopt;
    }

    return KiriView::ThumbnailOriginalIdentity::fromNonFileUri(
        uri, 0, metadata.uncompressedSize, QString());
}

QByteArray defaultThumbnailGenerationBytesLoader(
    const KiriView::ThumbnailGenerationRequest &request, QString *errorString)
{
    if (!request.openedCollectionScope.isEmpty()) {
        KiriView::MediaEntrySourceImageDataResult dataResult
            = loadOpenedCollectionThumbnailBytes(request);
        if (const auto *error = std::get_if<KiriView::MediaEntrySourceError>(&dataResult)) {
            if (errorString != nullptr) {
                *errorString = error->errorString;
            }
            return {};
        }
        if (auto *data = std::get_if<KiriView::MediaEntrySourceImageData>(&dataResult)) {
            return std::move(data->data);
        }
        if (errorString != nullptr) {
            *errorString = QStringLiteral("collection thumbnail bytes are unavailable");
        }
        return {};
    }

    QFile file(QFile::decodeName(request.localPathBytes));
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorString != nullptr) {
            *errorString = file.errorString();
        }
        return {};
    }

    return file.readAll();
}

std::optional<KiriView::ThumbnailOriginalIdentity> defaultOpenedCollectionOriginalIdentityLoader(
    const KiriView::ThumbnailGenerationRequest &request, QString *errorString)
{
    KiriView::MediaEntrySourceThumbnailMetadataResult metadataResult
        = loadOpenedCollectionThumbnailMetadata(request);
    if (const auto *error = std::get_if<KiriView::MediaEntrySourceError>(&metadataResult)) {
        if (errorString != nullptr) {
            *errorString = error->errorString;
        }
        return std::nullopt;
    }

    const auto *metadata
        = std::get_if<KiriView::MediaEntrySourceThumbnailMetadata>(&metadataResult);
    if (metadata == nullptr) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("collection thumbnail metadata failed");
        }
        return std::nullopt;
    }

    std::optional<KiriView::ThumbnailOriginalIdentity> identity
        = openedCollectionVirtualOriginalIdentity(*metadata);
    if (!identity.has_value() && errorString != nullptr) {
        *errorString = QStringLiteral("collection thumbnail identity failed");
    }
    return identity;
}

std::optional<KiriView::ThumbnailCacheLookupResult> lookupOpenedCollectionThumbnail(
    const KiriView::ThumbnailOriginalIdentity &identity,
    KiriView::ActiveNavigationThumbnailDemandBucket requestedBucket)
{
    const QByteArray uri = identity.uri.toUtf8();
    const QByteArray mimeType = identity.mimeType.toUtf8();
    const KiriView::RustThumbnailCacheLookupResult rustResult
        = KiriView::rustLookupDisplayThumbnailNonFileUriRgba8(KiriView::Bridge::rustStr(uri),
            identity.mtimeSeconds, identity.originalByteSize, KiriView::Bridge::rustStr(mimeType),
            rustBucket(requestedBucket));
    return lookupResultFromRust(rustResult);
}

KiriView::RustThumbnailCacheInstallResult installThumbnail(
    const KiriView::ThumbnailOriginalIdentity &identity,
    KiriView::ActiveNavigationThumbnailDemandBucket requestedBucket, const QImage &rgba8)
{
    const rust::Slice<const std::uint8_t> pixels(
        reinterpret_cast<const std::uint8_t *>(rgba8.constBits()),
        static_cast<std::size_t>(rgba8.sizeInBytes()));
    if (identity.isNonFileUri()) {
        const QByteArray uri = identity.uri.toUtf8();
        const QByteArray mimeType = identity.mimeType.toUtf8();
        return KiriView::rustInstallDisplayThumbnailNonFileUriRgba8(KiriView::Bridge::rustStr(uri),
            identity.mtimeSeconds, identity.originalByteSize, KiriView::Bridge::rustStr(mimeType),
            rustBucket(requestedBucket), rgba8.width(), rgba8.height(), rgba8.bytesPerLine(),
            pixels);
    }

    return KiriView::rustInstallDisplayThumbnailRgba8(
        KiriView::Bridge::rustBytes(identity.localPathBytes), rustBucket(requestedBucket),
        rgba8.width(), rgba8.height(), rgba8.bytesPerLine(), pixels);
}

KiriView::ThumbnailGenerationDependencies resolvedThumbnailGenerationDependencies(
    KiriView::ThumbnailGenerationDependencies dependencies)
{
    if (!dependencies.bytesLoader) {
        dependencies.bytesLoader = defaultThumbnailGenerationBytesLoader;
    }
    if (!dependencies.openedCollectionOriginalIdentityLoader) {
        dependencies.openedCollectionOriginalIdentityLoader
            = defaultOpenedCollectionOriginalIdentityLoader;
    }
    return dependencies;
}

KiriView::ThumbnailGenerationResult generateThumbnailWithDependencies(
    const KiriView::ThumbnailGenerationRequest &request,
    KiriView::ThumbnailGenerationDependencies dependencies)
{
    const int maximumLongEdge = bucketMaxEdge(request.requestedBucket);
    if (maximumLongEdge <= 0) {
        return failedResult(
            request.requestedBucket, QStringLiteral("thumbnail generation requires a size bucket"));
    }

    KiriView::ThumbnailOriginalIdentity originalIdentity = request.originalIdentity.isValid()
        ? request.originalIdentity
        : KiriView::ThumbnailOriginalIdentity::fromLocalPathBytes(request.localPathBytes);
    if (!request.openedCollectionScope.isEmpty()) {
        QString identityError;
        const std::optional<KiriView::ThumbnailOriginalIdentity> virtualIdentity
            = dependencies.openedCollectionOriginalIdentityLoader(request, &identityError);
        if (!virtualIdentity.has_value()) {
            return failedResult(request.requestedBucket,
                identityError.isEmpty() ? QStringLiteral("collection thumbnail identity failed")
                                        : std::move(identityError));
        }
        originalIdentity = *virtualIdentity;
        if (request.cacheInstallEnabled) {
            const std::optional<KiriView::ThumbnailCacheLookupResult> lookup
                = lookupOpenedCollectionThumbnail(originalIdentity, request.requestedBucket);
            if (lookup.has_value()
                && lookup->status == KiriView::ThumbnailCacheLookupStatus::Ready) {
                return readyResultFromCache(*lookup);
            }
            if (lookup.has_value()
                && (lookup->status == KiriView::ThumbnailCacheLookupStatus::Invalid
                    || lookup->status == KiriView::ThumbnailCacheLookupStatus::Failed)) {
                return failedResult(request.requestedBucket, lookup->errorString);
            }
        }
    }

    QString loadError;
    QByteArray bytes = dependencies.bytesLoader(request, &loadError);
    if (bytes.isEmpty() && !loadError.isEmpty()) {
        return failedResult(request.requestedBucket, std::move(loadError));
    }

    KiriView::DecodedImageResult decodeResult = KiriView::decodeImageData(bytes);
    if (const KiriView::DecodedImageFailure *failure = decodeResult.failure()) {
        return failedResult(request.requestedBucket, failure->errorString);
    }

    const KiriView::DecodedImage *decoded = decodeResult.image();
    if (decoded == nullptr) {
        return failedResult(
            request.requestedBucket, QStringLiteral("image decode produced no image"));
    }

    QString renderError;
    QImage image = renderedThumbnailImage(*decoded, maximumLongEdge, &renderError);
    if (image.isNull()) {
        return failedResult(request.requestedBucket,
            renderError.isEmpty() ? QStringLiteral("thumbnail render produced no image")
                                  : std::move(renderError));
    }

    QImage rgba8 = image.convertToFormat(QImage::Format_RGBA8888);
    if (rgba8.isNull()) {
        return failedResult(
            request.requestedBucket, QStringLiteral("thumbnail image conversion failed"));
    }

    if (!request.cacheInstallEnabled) {
        return KiriView::ThumbnailGenerationResult {
            KiriView::ThumbnailGenerationStatus::Ready,
            std::move(rgba8),
            request.requestedBucket,
            {},
            {},
        };
    }

    const KiriView::RustThumbnailCacheInstallResult install
        = installThumbnail(originalIdentity, request.requestedBucket, rgba8);
    if (!install.success) {
        return failedResult(
            thumbnailBucket(install.requested_bucket), KiriView::Bridge::qtString(install.error));
    }

    return KiriView::ThumbnailGenerationResult {
        KiriView::ThumbnailGenerationStatus::Ready,
        std::move(rgba8),
        thumbnailBucket(install.requested_bucket),
        KiriView::Bridge::qtString(install.installed_cache_path),
        {},
    };
}
}

namespace KiriView {
ThumbnailGenerationDependencies defaultThumbnailGenerationDependencies()
{
    return ThumbnailGenerationDependencies {
        defaultThumbnailGenerationBytesLoader,
        defaultOpenedCollectionOriginalIdentityLoader,
    };
}

ThumbnailGenerationResult generateThumbnail(
    const ThumbnailGenerationRequest &request, ThumbnailGenerationDependencies dependencies)
{
    return generateThumbnailWithDependencies(
        request, resolvedThumbnailGenerationDependencies(std::move(dependencies)));
}

ThumbnailGenerationProvider defaultThumbnailGenerationProvider(
    ImageWorkerScheduler workerScheduler, ThumbnailGenerationDependencies dependencies)
{
    return [workerScheduler = std::move(workerScheduler), dependencies = std::move(dependencies)](
               QObject *receiver, ThumbnailGenerationRequest request,
               ThumbnailGenerationCallback callback) {
        return startImageIoWorkerJob(
            receiver, workerScheduler,
            [request = std::move(request), dependencies]() {
                return generateThumbnail(request, dependencies);
            },
            [callback = std::move(callback)](ThumbnailGenerationResult result) mutable {
                if (callback) {
                    callback(std::move(result));
                }
            });
    };
}
}
