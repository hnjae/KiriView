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

int bucketMaxEdge(kiriview::ActiveNavigationThumbnailDemandBucket bucket)
{
    switch (bucket) {
    case kiriview::ActiveNavigationThumbnailDemandBucket::Normal:
        return 128;
    case kiriview::ActiveNavigationThumbnailDemandBucket::Large:
        return 256;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XLarge:
        return 512;
    case kiriview::ActiveNavigationThumbnailDemandBucket::XXLarge:
        return 1024;
    case kiriview::ActiveNavigationThumbnailDemandBucket::None:
        break;
    }

    return 0;
}

kiriview::ThumbnailGenerationResult failedResult(
    kiriview::ActiveNavigationThumbnailDemandBucket bucket, QString errorString)
{
    return kiriview::ThumbnailGenerationResult {
        kiriview::ThumbnailGenerationStatus::Failed,
        {},
        bucket,
        {},
        std::move(errorString),
    };
}

kiriview::ThumbnailCacheLookupStatus thumbnailCacheStatus(
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

kiriview::ThumbnailCacheLookupResult lookupResultFromRust(
    const kiriview::RustThumbnailCacheLookupResult& rustResult)
{
    kiriview::ThumbnailCacheLookupResult result;
    result.status = thumbnailCacheStatus(rustResult.status);
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

kiriview::ThumbnailGenerationResult readyResultFromCache(
    const kiriview::ThumbnailCacheLookupResult& lookup)
{
    return kiriview::ThumbnailGenerationResult {
        kiriview::ThumbnailGenerationStatus::Ready,
        lookup.image,
        lookup.requestedBucket,
        lookup.sourceCachePath,
        {},
    };
}

QSize boundedSize(QSize size, int maximumLongEdge)
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
    const kiriview::DecodedImage& decoded, int maximumLongEdge, QString* errorString)
{
    return std::visit(
        [maximumLongEdge, errorString](const auto& image) -> QImage {
            using Image = std::decay_t<decltype(image)>;
            if constexpr (std::is_same_v<Image, kiriview::StaticDecodedImage>) {
                if (image.displayImage.refinementSource == nullptr) {
                    if (errorString != nullptr) {
                        *errorString = QStringLiteral("static image source is unavailable");
                    }
                    return {};
                }
                kiriview::ImageTileSourceDisplayDecodeResult result
                    = image.displayImage.refinementSource
                          ->decodeBlockingDisplayImageWithDiagnostics(maximumLongEdge);
                if (result.image.isNull() && errorString != nullptr) {
                    *errorString = result.diagnostics.userMessage();
                }
                return result.image;
            } else {
                Q_UNUSED(errorString)
                return thumbnailFrame(image.firstFrame, maximumLongEdge);
            }
        },
        decoded);
}

QImage defaultThumbnailGenerationImageDecoder(
    QByteArray bytes, int maximumLongEdge, QString* errorString)
{
    kiriview::DecodedImageResult decodeResult = kiriview::decodeImageData(bytes);
    if (const kiriview::DecodedImageFailure* failure = decodeResult.failure()) {
        if (errorString != nullptr) {
            *errorString = failure->errorString;
        }
        return {};
    }

    const kiriview::DecodedImage* decoded = decodeResult.image();
    if (decoded == nullptr) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("image decode produced no image");
        }
        return {};
    }

    return renderedThumbnailImage(*decoded, maximumLongEdge, errorString);
}

kiriview::MediaEntrySourceImageDataResult loadOpenedCollectionThumbnailBytes(
    const kiriview::ThumbnailGenerationRequest& request)
{
    return kiriview::loadMediaEntrySourceImageData(
        request.openedCollectionScope, request.sourceUrl);
}

kiriview::MediaEntrySourceThumbnailMetadataResult loadOpenedCollectionThumbnailMetadata(
    const kiriview::ThumbnailGenerationRequest& request)
{
    return kiriview::loadMediaEntrySourceThumbnailMetadata(
        request.openedCollectionScope, request.sourceUrl);
}

std::optional<kiriview::ThumbnailOriginalIdentity> openedCollectionVirtualOriginalIdentity(
    const kiriview::MediaEntrySourceThumbnailMetadata& metadata)
{
    if (metadata.checksum.algorithm != kiriview::MediaEntryContentChecksumAlgorithm::Crc32
        || metadata.checksum.value > std::numeric_limits<std::uint32_t>::max()
        || metadata.uncompressedSize < 0) {
        return std::nullopt;
    }

    const QString uri
        = kiriview::Bridge::qtString(kiriview::rustVirtualThumbnailArchiveEntryCrc32Uri(
            static_cast<std::uint32_t>(metadata.checksum.value),
            static_cast<std::uint64_t>(metadata.uncompressedSize)));
    if (uri.isEmpty()) {
        return std::nullopt;
    }

    return kiriview::ThumbnailOriginalIdentity::fromNonFileUri(
        uri, 0, metadata.uncompressedSize, QString());
}

QByteArray defaultThumbnailGenerationBytesLoader(
    const kiriview::ThumbnailGenerationRequest& request, QString* errorString)
{
    if (!request.openedCollectionScope.isEmpty()) {
        kiriview::MediaEntrySourceImageDataResult dataResult
            = loadOpenedCollectionThumbnailBytes(request);
        if (const auto* error = std::get_if<kiriview::MediaEntrySourceError>(&dataResult)) {
            if (errorString != nullptr) {
                *errorString = error->errorString;
            }
            return {};
        }
        if (auto* data = std::get_if<kiriview::MediaEntrySourceImageData>(&dataResult)) {
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

std::optional<kiriview::ThumbnailOriginalIdentity> defaultOpenedCollectionOriginalIdentityLoader(
    const kiriview::ThumbnailGenerationRequest& request, QString* errorString)
{
    kiriview::MediaEntrySourceThumbnailMetadataResult metadataResult
        = loadOpenedCollectionThumbnailMetadata(request);
    if (const auto* error = std::get_if<kiriview::MediaEntrySourceError>(&metadataResult)) {
        if (errorString != nullptr) {
            *errorString = error->errorString;
        }
        return std::nullopt;
    }

    const auto* metadata
        = std::get_if<kiriview::MediaEntrySourceThumbnailMetadata>(&metadataResult);
    if (metadata == nullptr) {
        if (errorString != nullptr) {
            *errorString = QStringLiteral("collection thumbnail metadata failed");
        }
        return std::nullopt;
    }

    std::optional<kiriview::ThumbnailOriginalIdentity> identity
        = openedCollectionVirtualOriginalIdentity(*metadata);
    if (!identity.has_value() && errorString != nullptr) {
        *errorString = QStringLiteral("collection thumbnail identity failed");
    }
    return identity;
}

std::optional<kiriview::ThumbnailCacheLookupResult> defaultThumbnailGenerationCacheLookup(
    const kiriview::ThumbnailOriginalIdentity& identity,
    kiriview::ActiveNavigationThumbnailDemandBucket requestedBucket)
{
    kiriview::RustThumbnailCacheLookupResult rustResult;
    if (identity.isNonFileUri()) {
        const QByteArray uri = identity.uri.toUtf8();
        const QByteArray mimeType = identity.mimeType.toUtf8();
        rustResult = kiriview::rustLookupDisplayThumbnailNonFileUriRgba8(
            kiriview::Bridge::rustStr(uri), identity.mtimeSeconds, identity.originalByteSize,
            kiriview::Bridge::rustStr(mimeType), rustBucket(requestedBucket));
    } else {
        rustResult = kiriview::rustLookupDisplayThumbnailRgba8(
            kiriview::Bridge::rustBytes(identity.localPathBytes), rustBucket(requestedBucket));
    }
    return lookupResultFromRust(rustResult);
}

kiriview::ThumbnailGenerationCacheInstallResult installThumbnail(
    const kiriview::ThumbnailOriginalIdentity& identity,
    kiriview::ActiveNavigationThumbnailDemandBucket requestedBucket, const QImage& rgba8)
{
    const rust::Slice<const std::uint8_t> pixels(
        reinterpret_cast<const std::uint8_t*>(rgba8.constBits()),
        static_cast<std::size_t>(rgba8.sizeInBytes()));
    kiriview::RustThumbnailCacheInstallResult install;
    if (identity.isNonFileUri()) {
        const QByteArray uri = identity.uri.toUtf8();
        const QByteArray mimeType = identity.mimeType.toUtf8();
        install = kiriview::rustInstallDisplayThumbnailNonFileUriRgba8(
            kiriview::Bridge::rustStr(uri), identity.mtimeSeconds, identity.originalByteSize,
            kiriview::Bridge::rustStr(mimeType), rustBucket(requestedBucket), rgba8.width(),
            rgba8.height(), rgba8.bytesPerLine(), pixels);
    } else {
        install = kiriview::rustInstallDisplayThumbnailRgba8(
            kiriview::Bridge::rustBytes(identity.localPathBytes), rustBucket(requestedBucket),
            rgba8.width(), rgba8.height(), rgba8.bytesPerLine(), pixels);
    }

    return kiriview::ThumbnailGenerationCacheInstallResult {
        install.success,
        thumbnailBucket(install.requested_bucket),
        kiriview::Bridge::qtString(install.installed_cache_path),
        kiriview::Bridge::qtString(install.error),
    };
}

kiriview::ThumbnailGenerationResult finishGeneratedThumbnailImage(
    const kiriview::ThumbnailGenerationRequest& request,
    const kiriview::ThumbnailOriginalIdentity& originalIdentity, QImage image,
    const kiriview::ThumbnailGenerationDependencies& dependencies)
{
    QImage rgba8 = image.convertToFormat(QImage::Format_RGBA8888);
    if (rgba8.isNull()) {
        return failedResult(
            request.requestedBucket, QStringLiteral("thumbnail image conversion failed"));
    }

    if (!request.cacheInstallEnabled) {
        return kiriview::ThumbnailGenerationResult {
            kiriview::ThumbnailGenerationStatus::Ready,
            std::move(rgba8),
            request.requestedBucket,
            {},
            {},
        };
    }

    const kiriview::ThumbnailGenerationCacheInstallResult install
        = dependencies.cacheRepository.install(originalIdentity, request.requestedBucket, rgba8);
    if (!install.success) {
        return failedResult(install.requestedBucket, install.errorString);
    }

    return kiriview::ThumbnailGenerationResult {
        kiriview::ThumbnailGenerationStatus::Ready,
        std::move(rgba8),
        install.requestedBucket,
        install.installedCachePath,
        {},
    };
}

kiriview::ThumbnailGenerationDependencies resolvedThumbnailGenerationDependencies(
    kiriview::ThumbnailGenerationDependencies dependencies)
{
    if (!dependencies.bytesLoader) {
        dependencies.bytesLoader = defaultThumbnailGenerationBytesLoader;
    }
    if (!dependencies.imageDecoder) {
        dependencies.imageDecoder = defaultThumbnailGenerationImageDecoder;
    }
    if (!dependencies.maximumLongEdgeForBucket) {
        dependencies.maximumLongEdgeForBucket = bucketMaxEdge;
    }
    if (!dependencies.openedCollectionOriginalIdentityLoader) {
        dependencies.openedCollectionOriginalIdentityLoader
            = defaultOpenedCollectionOriginalIdentityLoader;
    }
    if (!dependencies.cacheRepository.lookup) {
        dependencies.cacheRepository.lookup = defaultThumbnailGenerationCacheLookup;
    }
    if (!dependencies.cacheRepository.install) {
        dependencies.cacheRepository.install = installThumbnail;
    }
    if (!dependencies.videoExtractor) {
        dependencies.videoExtractor = kiriview::startVideoThumbnailExtraction;
    }
    return dependencies;
}

kiriview::ThumbnailGenerationResult generateThumbnailWithDependencies(
    const kiriview::ThumbnailGenerationRequest& request,
    kiriview::ThumbnailGenerationDependencies dependencies)
{
    const int maximumLongEdge = dependencies.maximumLongEdgeForBucket(request.requestedBucket);
    if (maximumLongEdge <= 0) {
        return failedResult(
            request.requestedBucket, QStringLiteral("thumbnail generation requires a size bucket"));
    }

    kiriview::ThumbnailOriginalIdentity originalIdentity = request.originalIdentity.isValid()
        ? request.originalIdentity
        : kiriview::ThumbnailOriginalIdentity::fromLocalPathBytes(request.localPathBytes);
    if (!request.openedCollectionScope.isEmpty()) {
        QString identityError;
        const std::optional<kiriview::ThumbnailOriginalIdentity> virtualIdentity
            = dependencies.openedCollectionOriginalIdentityLoader(request, &identityError);
        if (!virtualIdentity.has_value()) {
            return failedResult(request.requestedBucket,
                identityError.isEmpty() ? QStringLiteral("collection thumbnail identity failed")
                                        : std::move(identityError));
        }
        originalIdentity = *virtualIdentity;
        if (request.cacheInstallEnabled) {
            const std::optional<kiriview::ThumbnailCacheLookupResult> lookup
                = dependencies.cacheRepository.lookup(originalIdentity, request.requestedBucket);
            if (lookup.has_value()
                && lookup->status == kiriview::ThumbnailCacheLookupStatus::Ready) {
                return readyResultFromCache(*lookup);
            }
            if (lookup.has_value()
                && (lookup->status == kiriview::ThumbnailCacheLookupStatus::Invalid
                    || lookup->status == kiriview::ThumbnailCacheLookupStatus::Failed)) {
                return failedResult(request.requestedBucket, lookup->errorString);
            }
        }
    }

    QString loadError;
    QByteArray bytes = dependencies.bytesLoader(request, &loadError);
    if (bytes.isEmpty() && !loadError.isEmpty()) {
        return failedResult(request.requestedBucket, std::move(loadError));
    }

    QString decodeError;
    QImage image = dependencies.imageDecoder(std::move(bytes), maximumLongEdge, &decodeError);
    if (image.isNull()) {
        return failedResult(request.requestedBucket,
            decodeError.isEmpty() ? QStringLiteral("thumbnail render produced no image")
                                  : std::move(decodeError));
    }

    return finishGeneratedThumbnailImage(request, originalIdentity, std::move(image), dependencies);
}

kiriview::ImageIoJob startVideoThumbnailGenerationJob(QObject* receiver,
    kiriview::ThumbnailGenerationRequest request, kiriview::ThumbnailGenerationCallback callback,
    kiriview::ThumbnailGenerationDependencies dependencies)
{
    const int maximumLongEdge = dependencies.maximumLongEdgeForBucket(request.requestedBucket);
    if (maximumLongEdge <= 0) {
        if (callback) {
            callback(failedResult(request.requestedBucket,
                QStringLiteral("thumbnail generation requires a size bucket")));
        }
        return {};
    }

    kiriview::ThumbnailOriginalIdentity originalIdentity = request.originalIdentity.isValid()
        ? request.originalIdentity
        : kiriview::ThumbnailOriginalIdentity::fromLocalPathBytes(request.localPathBytes);
    if (!originalIdentity.isValid()) {
        if (callback) {
            callback(failedResult(
                request.requestedBucket, QStringLiteral("video thumbnail identity failed")));
        }
        return {};
    }

    kiriview::VideoThumbnailExtractionRequest extractionRequest;
    extractionRequest.localPathBytes = request.localPathBytes;
    extractionRequest.sourceUrl = request.sourceUrl;
    extractionRequest.requestedBucket = request.requestedBucket;
    extractionRequest.maximumLongEdge = maximumLongEdge;

    kiriview::VideoThumbnailExtractionProvider videoExtractor = dependencies.videoExtractor;
    return videoExtractor(receiver, std::move(extractionRequest),
        [request = std::move(request), originalIdentity = std::move(originalIdentity),
            callback = std::move(callback), dependencies = std::move(dependencies)](
            kiriview::VideoThumbnailExtractionResult extractionResult) mutable {
            if (!callback) {
                return;
            }
            if (extractionResult.status == kiriview::ThumbnailGenerationStatus::Failed) {
                callback(failedResult(request.requestedBucket,
                    extractionResult.errorString.isEmpty()
                        ? QStringLiteral("video thumbnail extraction failed")
                        : std::move(extractionResult.errorString)));
                return;
            }
            if (extractionResult.image.isNull()) {
                callback(failedResult(
                    request.requestedBucket, QStringLiteral("video thumbnail produced no image")));
                return;
            }

            callback(finishGeneratedThumbnailImage(
                request, originalIdentity, std::move(extractionResult.image), dependencies));
        });
}
}

namespace kiriview {
ThumbnailGenerationDependencies defaultThumbnailGenerationDependencies()
{
    return ThumbnailGenerationDependencies {
        defaultThumbnailGenerationBytesLoader,
        defaultThumbnailGenerationImageDecoder,
        bucketMaxEdge,
        defaultOpenedCollectionOriginalIdentityLoader,
        ThumbnailGenerationCacheRepository {
            defaultThumbnailGenerationCacheLookup, installThumbnail },
        startVideoThumbnailExtraction,
    };
}

ThumbnailGenerationResult generateThumbnail(
    const ThumbnailGenerationRequest& request, ThumbnailGenerationDependencies dependencies)
{
    return generateThumbnailWithDependencies(
        request, resolvedThumbnailGenerationDependencies(std::move(dependencies)));
}

ThumbnailGenerationProvider defaultThumbnailGenerationProvider(
    ImageWorkerScheduler workerScheduler, ThumbnailGenerationDependencies dependencies)
{
    return [workerScheduler = std::move(workerScheduler), dependencies = std::move(dependencies)](
               QObject* receiver, ThumbnailGenerationRequest request,
               ThumbnailGenerationCallback callback) {
        ThumbnailGenerationDependencies resolvedDependencies
            = resolvedThumbnailGenerationDependencies(dependencies);
        if (request.sourceKind == ThumbnailSourceKind::DirectVideo) {
            return startVideoThumbnailGenerationJob(
                receiver, std::move(request), std::move(callback), std::move(resolvedDependencies));
        }

        return startImageIoWorkerJob(
            receiver, workerScheduler,
            [request = std::move(request), dependencies = std::move(resolvedDependencies)]() {
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
