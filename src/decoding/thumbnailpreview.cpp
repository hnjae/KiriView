// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnailpreview.h"

#include "bufferedimagereader.h"
#include "imageinputclassification.h"
#include "location/sourcekey.h"
#include "rendering/heiftilesource.h"
#include "rendering/imagerendering.h"
#include "rendering/svgtilesource.h"

#include <QFile>
#include <QImageIOHandler>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
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

QSize transformedImageSize(const QSize &size, QImageIOHandler::Transformations transformations)
{
    if (size.isEmpty()) {
        return {};
    }
    if (transformations.toInt() & QImageIOHandler::TransformationRotate90) {
        return size.transposed();
    }
    return size;
}

std::optional<QSize> qtRasterTrustedOriginalSize(
    const QByteArray &data, KiriView::QtRasterFormat format)
{
    KiriView::BufferedImageReader reader(data, KiriView::qtImageReaderFormat(format));
    if (!reader || reader.supportsAnimation()) {
        return std::nullopt;
    }

    const QSize size = transformedImageSize(reader.size(), reader.transformation());
    if (!validImageSize(size)) {
        return std::nullopt;
    }
    return size;
}

std::optional<QSize> svgTrustedOriginalSize(const QByteArray &data)
{
    QString errorString;
    const std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    if (source == nullptr || !validImageSize(source->imageSize())) {
        return std::nullopt;
    }
    return source->imageSize();
}

std::optional<QSize> heifTrustedOriginalSize(const QByteArray &data)
{
    QString errorString;
    const std::shared_ptr<KiriView::ImageTileSource> source
        = KiriView::openHeifTileSource(data, &errorString);
    if (source == nullptr || !validImageSize(source->imageSize())) {
        return std::nullopt;
    }
    return source->imageSize();
}

std::optional<QSize> trustedOriginalSizeForDecodeData(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    const KiriView::ImageInputClassification classification
        = KiriView::classifyImageInput(data, request.imageUrl().fileName());
    switch (classification.kind) {
    case KiriView::ImageInputKind::Svg:
        return svgTrustedOriginalSize(data);
    case KiriView::ImageInputKind::HeifFamily:
        return heifTrustedOriginalSize(data);
    case KiriView::ImageInputKind::QtRaster:
        return qtRasterTrustedOriginalSize(data, classification.qtFormat);
    case KiriView::ImageInputKind::Apng:
    case KiriView::ImageInputKind::Raw:
    case KiriView::ImageInputKind::Unknown:
        break;
    }

    return std::nullopt;
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

std::optional<XdgThumbnailPreviewRequest> xdgThumbnailPreviewRequestForDecodeData(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    if (request.isEmpty()) {
        return std::nullopt;
    }

    std::optional<QSize> trustedOriginalSize = trustedOriginalSizeForDecodeData(data, request);
    if (!trustedOriginalSize.has_value()) {
        return std::nullopt;
    }

    return XdgThumbnailPreviewRequest {
        request.imageUrl(),
        *trustedOriginalSize,
        ActiveNavigationThumbnailDemandBucket::XXLarge,
        true,
    };
}

std::optional<StaticDisplayImagePayload> xdgThumbnailPreviewDisplayPayload(
    const ImageDecodeRequest &request, XdgThumbnailPreviewResult result)
{
    if (result.status != ThumbnailCacheLookupStatus::Ready || result.image.isNull()
        || !validImageSize(result.originalSize)) {
        return std::nullopt;
    }

    QImage image = displayReadyImage(std::move(result.image));
    const qreal pixelsPerSourcePixel = imagePixelsPerSourcePixel(result.originalSize, image.size());
    return StaticDisplayImagePayload {
        sourceKeyForUrl(request.imageUrl()).identity,
        {},
        result.originalSize,
        std::move(image),
        DisplayImageQuality::ThumbnailPreview,
        pixelsPerSourcePixel > 0.0 ? pixelsPerSourcePixel : 0.0,
        {},
        nullptr,
        DisplayImagePreviewOrigin::XdgThumbnail,
    };
}
}
