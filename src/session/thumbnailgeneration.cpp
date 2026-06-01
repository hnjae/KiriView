// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailgeneration.h"

#include "async/imageioworkerjob.h"
#include "bridge/rustqtconversion.h"
#include "decoding/decodedimageresult.h"
#include "decoding/kiriimagedecoder.h"
#include "kiriview/src/policy/thumbnailcache.cxx.h"
#include "rendering/staticimage.h"

#include <QFile>
#include <QImage>
#include <QSize>
#include <Qt>
#include <algorithm>
#include <cstdint>
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
                if (image.staticImage.source == nullptr) {
                    if (errorString != nullptr) {
                        *errorString = QStringLiteral("static image source is unavailable");
                    }
                    return {};
                }
                return image.staticImage.source->decodeBlockingDisplayImage(
                    maximumLongEdge, errorString);
            } else {
                Q_UNUSED(errorString)
                return thumbnailFrame(image.firstFrame, maximumLongEdge);
            }
        },
        decoded);
}

KiriView::ThumbnailGenerationResult generateThumbnail(
    const KiriView::ThumbnailGenerationRequest &request)
{
    const int maximumLongEdge = bucketMaxEdge(request.requestedBucket);
    if (maximumLongEdge <= 0) {
        return failedResult(
            request.requestedBucket, QStringLiteral("thumbnail generation requires a size bucket"));
    }

    QFile file(QFile::decodeName(request.localPathBytes));
    if (!file.open(QIODevice::ReadOnly)) {
        return failedResult(request.requestedBucket, file.errorString());
    }

    KiriView::DecodedImageResult decodeResult = KiriView::decodeImageData(file.readAll());
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

    const rust::Slice<const std::uint8_t> pixels(
        reinterpret_cast<const std::uint8_t *>(rgba8.constBits()),
        static_cast<std::size_t>(rgba8.sizeInBytes()));
    const KiriView::RustThumbnailCacheInstallResult install
        = KiriView::rustInstallDisplayThumbnailRgba8(
            KiriView::Bridge::rustBytes(request.localPathBytes),
            rustBucket(request.requestedBucket), rgba8.width(), rgba8.height(),
            rgba8.bytesPerLine(), pixels);
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
ThumbnailGenerationProvider defaultThumbnailGenerationProvider()
{
    return [](QObject *receiver, ThumbnailGenerationRequest request,
               ThumbnailGenerationCallback callback) {
        return startImageIoWorkerJob(
            receiver, [request = std::move(request)]() { return generateThumbnail(request); },
            [callback = std::move(callback)](ThumbnailGenerationResult result) mutable {
                if (callback) {
                    callback(std::move(result));
                }
            });
    };
}
}
