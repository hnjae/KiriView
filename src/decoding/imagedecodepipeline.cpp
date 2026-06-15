// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include "apnganimationreader.h"
#include "bridge/rustqtconversion.h"
#include "heifdecoder.h"
#include "jxlanimationreader.h"
#include "kiriview/src/policy/avifcompat.cxx.h"
#include "localization/imageerrortext.h"
#include "location/sourcekey.h"
#include "metadata/embeddedmetadata.h"
#include "qimagereaderdecoder.h"
#include "rawdecoder.h"
#include "rendering/svgtilesource.h"
#include "staticimagedecode.h"
#include "webpanimationreader.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QString>
#include <memory>
#include <optional>
#include <utility>

Q_LOGGING_CATEGORY(kiriviewDecodeLog, "org.hnjae.kiriview.decode", QtWarningMsg)

namespace {
const char *imageInputKindName(kiriview::ImageInputKind kind)
{
    switch (kind) {
    case kiriview::ImageInputKind::Unknown:
        return "Unknown";
    case kiriview::ImageInputKind::Svg:
        return "Svg";
    case kiriview::ImageInputKind::Apng:
        return "Apng";
    case kiriview::ImageInputKind::HeifFamily:
        return "HeifFamily";
    case kiriview::ImageInputKind::Raw:
        return "Raw";
    case kiriview::ImageInputKind::QtRaster:
        return "QtRaster";
    }

    return "Unknown";
}

const char *imageDecodeHandlerKindName(kiriview::ImageDecodeHandlerKind kind)
{
    switch (kind) {
    case kiriview::ImageDecodeHandlerKind::None:
        return "None";
    case kiriview::ImageDecodeHandlerKind::Svg:
        return "Svg";
    case kiriview::ImageDecodeHandlerKind::Apng:
        return "Apng";
    case kiriview::ImageDecodeHandlerKind::HeifFamily:
        return "HeifFamily";
    case kiriview::ImageDecodeHandlerKind::Raw:
        return "Raw";
    case kiriview::ImageDecodeHandlerKind::QtRaster:
        return "QtRaster";
    }

    return "None";
}

kiriview::DecodedImageFailureRoute decodedFailureRouteForHandlerKind(
    kiriview::ImageDecodeHandlerKind kind)
{
    switch (kind) {
    case kiriview::ImageDecodeHandlerKind::Svg:
        return kiriview::DecodedImageFailureRoute::Svg;
    case kiriview::ImageDecodeHandlerKind::Apng:
        return kiriview::DecodedImageFailureRoute::Apng;
    case kiriview::ImageDecodeHandlerKind::HeifFamily:
        return kiriview::DecodedImageFailureRoute::HeifFamily;
    case kiriview::ImageDecodeHandlerKind::Raw:
        return kiriview::DecodedImageFailureRoute::Raw;
    case kiriview::ImageDecodeHandlerKind::QtRaster:
        return kiriview::DecodedImageFailureRoute::QtRaster;
    case kiriview::ImageDecodeHandlerKind::None:
        return kiriview::DecodedImageFailureRoute::Unknown;
    }

    return kiriview::DecodedImageFailureRoute::Unknown;
}

const char *imageDecodeDataSourceName(kiriview::ImageDecodeDataSource source)
{
    switch (source) {
    case kiriview::ImageDecodeDataSource::Original:
        return "Original";
    case kiriview::ImageDecodeDataSource::AvifCompatible:
        return "AvifCompatible";
    }

    return "Original";
}

const char *qtRasterFormatName(kiriview::QtRasterFormat format)
{
    switch (format) {
    case kiriview::QtRasterFormat::None:
        return "None";
    case kiriview::QtRasterFormat::Png:
        return "Png";
    case kiriview::QtRasterFormat::Jpeg:
        return "Jpeg";
    case kiriview::QtRasterFormat::Gif:
        return "Gif";
    case kiriview::QtRasterFormat::Webp:
        return "Webp";
    case kiriview::QtRasterFormat::Bmp:
        return "Bmp";
    case kiriview::QtRasterFormat::Tiff:
        return "Tiff";
    case kiriview::QtRasterFormat::Jxl:
        return "Jxl";
    case kiriview::QtRasterFormat::Jp2:
        return "Jp2";
    }

    return "None";
}

QByteArray avifCompatibleImageData(const QByteArray &data)
{
    return kiriview::Bridge::qtByteArray(
        kiriview::avifDataWithCompatibilityFixes(kiriview::Bridge::rustBytes(data)));
}

class ImageDecodeRouterByteInputs
{
public:
    ImageDecodeRouterByteInputs(const QByteArray &originalData,
        const kiriview::ImageDecodeCompatibleDataTransform &compatibleDataTransform)
        : m_originalData(originalData)
        , m_compatibleDataTransform(compatibleDataTransform)
    {
    }

    const QByteArray &dataFor(kiriview::ImageDecodeDataSource dataSource)
    {
        switch (dataSource) {
        case kiriview::ImageDecodeDataSource::Original:
            return m_originalData;
        case kiriview::ImageDecodeDataSource::AvifCompatible:
            return compatibleData();
        }

        return m_originalData;
    }

private:
    const QByteArray &compatibleData()
    {
        if (!m_compatibleData.has_value()) {
            m_compatibleData = m_compatibleDataTransform ? m_compatibleDataTransform(m_originalData)
                                                         : avifCompatibleImageData(m_originalData);
        }
        return *m_compatibleData;
    }

    const QByteArray &m_originalData;
    const kiriview::ImageDecodeCompatibleDataTransform &m_compatibleDataTransform;
    std::optional<QByteArray> m_compatibleData;
};

kiriview::DecodedImageResult failedReadImageDataResult()
{
    return kiriview::failedDecodedImageResult(
        kiriview::imageErrorText(kiriview::ImageErrorTextId::ReadImageData));
}

kiriview::DecodedImageResult failedImageDataResult(QString errorString)
{
    if (errorString.isEmpty()) {
        return failedReadImageDataResult();
    }
    return kiriview::failedDecodedImageResult(std::move(errorString));
}

kiriview::DecodedImageResult failedAnimationOpenResult(QString errorString, QString adapterName)
{
    const QString backendError = errorString;
    return kiriview::failedDecodedImageResult(kiriview::DecodedImageFailure {
        std::move(errorString),
        kiriview::DecodedImageFailureRoute::QtRaster,
        kiriview::DecodedImageFailureOperation::DecodeAnimationOpen,
        QStringLiteral("%1 animation open failed: %2")
            .arg(std::move(adapterName),
                backendError.isEmpty() ? QStringLiteral("<empty>") : backendError),
        kiriview::DecodedImageFailureSeverity::Error,
        false,
    });
}

QString sourceIdentityForRequest(const kiriview::ImageDecodeRequest &request)
{
    return kiriview::sourceKeyForUrl(request.imageUrl()).identity;
}

kiriview::DecodedImageResult decodeSvgImageData(const kiriview::ImageDecodeRouterInput &input)
{
    QString errorString;
    std::shared_ptr<kiriview::SvgTileSource> source
        = kiriview::SvgTileSource::open(input.data, &errorString);
    if (source == nullptr) {
        return failedImageDataResult(std::move(errorString));
    }

    return kiriview::staticDecodedImageResult(std::move(source), input.request, &errorString);
}

kiriview::DecodedImageResult decodeApngImageData(const kiriview::ImageDecodeRouterInput &input)
{
    kiriview::ApngAnimationReader apngReader;
    kiriview::ApngOpenResult apngResult = apngReader.open(input.data);
    if (apngResult.status == kiriview::ApngOpenStatus::NotApng) {
        return kiriview::failedDecodedImageResult(
            kiriview::imageErrorText(kiriview::ImageErrorTextId::DecodeApngAnimation));
    }
    if (apngResult.status == kiriview::ApngOpenStatus::Error) {
        return kiriview::failedDecodedImageResult(apngResult.errorString);
    }

    return kiriview::successfulDecodedImageResult(kiriview::ApngAnimationImage {
        std::move(apngResult.firstFrame),
        input.data,
        {},
        sourceIdentityForRequest(input.request),
    });
}

kiriview::DecodedImageResult decodeHeifRouterImageData(
    const kiriview::ImageDecodeRouterInput &input)
{
    std::optional<kiriview::DecodedImageResult> result
        = kiriview::decodeHeifImageData(input.data, input.request);
    if (!result.has_value()) {
        return failedReadImageDataResult();
    }
    return std::move(*result);
}

kiriview::DecodedImageResult decodeRawRouterImageData(const kiriview::ImageDecodeRouterInput &input)
{
    return kiriview::decodeRawImageData(input.data, input.request);
}

kiriview::DecodedImageResult decodeQImageReaderRouterImageData(
    const kiriview::ImageDecodeRouterInput &input)
{
    if (input.qtRasterFormat == kiriview::QtRasterFormat::Webp) {
        kiriview::WebPAnimationReader reader;
        kiriview::WebPAnimationOpenResult openResult = reader.open(input.data);
        switch (openResult.status) {
        case kiriview::WebPAnimationOpenStatus::Success:
            return kiriview::successfulDecodedImageResult(kiriview::WebPAnimationImage {
                std::move(openResult.firstFrame),
                input.data,
                {},
                sourceIdentityForRequest(input.request),
            });
        case kiriview::WebPAnimationOpenStatus::Error:
            return failedAnimationOpenResult(openResult.errorString, QStringLiteral("WebP"));
        case kiriview::WebPAnimationOpenStatus::NotWebP:
        case kiriview::WebPAnimationOpenStatus::NotAnimation:
            break;
        }
    }

    if (input.qtRasterFormat == kiriview::QtRasterFormat::Jxl) {
        kiriview::JxlAnimationReader reader;
        kiriview::JxlAnimationOpenResult openResult = reader.open(input.data);
        switch (openResult.status) {
        case kiriview::JxlAnimationOpenStatus::Success:
            return kiriview::successfulDecodedImageResult(kiriview::JxlAnimationImage {
                std::move(openResult.firstFrame),
                input.data,
                {},
                sourceIdentityForRequest(input.request),
            });
        case kiriview::JxlAnimationOpenStatus::Error:
            return failedAnimationOpenResult(openResult.errorString, QStringLiteral("JXL"));
        case kiriview::JxlAnimationOpenStatus::NotJxl:
        case kiriview::JxlAnimationOpenStatus::NotAnimation:
            break;
        }
    }

    return kiriview::decodeQImageReaderImageData(input.data, input.request, input.qtRasterFormat);
}

kiriview::ImageDecodeRouterHandlers defaultImageDecodeRouterHandlers()
{
    return kiriview::ImageDecodeRouterHandlers {
        decodeSvgImageData,
        decodeApngImageData,
        decodeHeifRouterImageData,
        decodeRawRouterImageData,
        decodeQImageReaderRouterImageData,
    };
}

kiriview::ImageDecodeRouterHandlers withDefaultHandlers(
    kiriview::ImageDecodeRouterHandlers handlers)
{
    const kiriview::ImageDecodeRouterHandlers defaults = defaultImageDecodeRouterHandlers();
    if (!handlers.svg) {
        handlers.svg = defaults.svg;
    }
    if (!handlers.apng) {
        handlers.apng = defaults.apng;
    }
    if (!handlers.heifFamily) {
        handlers.heifFamily = defaults.heifFamily;
    }
    if (!handlers.raw) {
        handlers.raw = defaults.raw;
    }
    if (!handlers.qtRaster) {
        handlers.qtRaster = defaults.qtRaster;
    }
    return handlers;
}

kiriview::DecodedImageResult dispatchToHandler(const kiriview::ImageDecodeRouterHandler &handler,
    const kiriview::ImageDecodeRouterInput &input, kiriview::ImageDecodeHandlerKind handlerKind)
{
    if (!handler) {
        return failedReadImageDataResult();
    }
    kiriview::DecodedImageResult result = handler(input);
    kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    if (failure != nullptr && failure->route == kiriview::DecodedImageFailureRoute::Unknown) {
        failure->route = decodedFailureRouteForHandlerKind(handlerKind);
    }
    return result;
}

const kiriview::ImageDecodeRouterHandler &emptyHandler()
{
    static const kiriview::ImageDecodeRouterHandler handler;
    return handler;
}

const kiriview::ImageDecodeRouterHandler &handlerForRoute(
    const kiriview::ImageDecodeRouterHandlers &handlers,
    kiriview::ImageDecodeHandlerKind handlerKind)
{
    switch (handlerKind) {
    case kiriview::ImageDecodeHandlerKind::Svg:
        return handlers.svg;
    case kiriview::ImageDecodeHandlerKind::Apng:
        return handlers.apng;
    case kiriview::ImageDecodeHandlerKind::HeifFamily:
        return handlers.heifFamily;
    case kiriview::ImageDecodeHandlerKind::Raw:
        return handlers.raw;
    case kiriview::ImageDecodeHandlerKind::QtRaster:
        return handlers.qtRaster;
    case kiriview::ImageDecodeHandlerKind::None:
        return emptyHandler();
    }

    return emptyHandler();
}
}

namespace kiriview {
ImageDecodeRoute imageDecodeRouteForClassification(ImageInputClassification classification)
{
    switch (classification.kind) {
    case ImageInputKind::Svg:
        return ImageDecodeRoute {
            ImageDecodeHandlerKind::Svg,
            classification.dataSource,
            classification.qtFormat,
        };
    case ImageInputKind::Apng:
        return ImageDecodeRoute {
            ImageDecodeHandlerKind::Apng,
            classification.dataSource,
            classification.qtFormat,
        };
    case ImageInputKind::HeifFamily:
        return ImageDecodeRoute {
            ImageDecodeHandlerKind::HeifFamily,
            classification.dataSource,
            classification.qtFormat,
        };
    case ImageInputKind::Raw:
        return ImageDecodeRoute {
            ImageDecodeHandlerKind::Raw,
            classification.dataSource,
            classification.qtFormat,
        };
    case ImageInputKind::QtRaster:
        return ImageDecodeRoute {
            ImageDecodeHandlerKind::QtRaster,
            classification.dataSource,
            classification.qtFormat,
        };
    case ImageInputKind::Unknown:
        return {};
    }

    return {};
}

ImageDecodeRouterRuntime::ImageDecodeRouterRuntime(
    ImageDecodeRouterHandlers handlers, ImageDecodeCompatibleDataTransform compatibleDataTransform)
    : m_handlers(withDefaultHandlers(std::move(handlers)))
    , m_compatibleDataTransform(std::move(compatibleDataTransform))
{
}

DecodedImageResult ImageDecodeRouterRuntime::execute(
    const ImageDecodeRoute &route, const QByteArray &data, const ImageDecodeRequest &request) const
{
    if (!route.shouldDecode()) {
        return failedReadImageDataResult();
    }

    ImageDecodeRouterByteInputs byteInputs(data, m_compatibleDataTransform);
    const ImageDecodeRouterInput input {
        byteInputs.dataFor(route.dataSource),
        request,
        route.qtRasterFormat,
    };

    return dispatchToHandler(
        handlerForRoute(m_handlers, route.handlerKind), input, route.handlerKind);
}

ImageDecodeRouter::ImageDecodeRouter(ImageDecodeRouterHandlers handlers,
    ImageDecodeInputClassifier classifier,
    ImageDecodeCompatibleDataTransform compatibleDataTransform)
    : m_classifier(std::move(classifier))
    , m_runtime(std::move(handlers), std::move(compatibleDataTransform))
{
    if (!m_classifier) {
        m_classifier = classifyImageInput;
    }
}

DecodedImageResult ImageDecodeRouter::decode(
    const QByteArray &data, const ImageDecodeRequest &request) const
{
    const ImageInputClassification classification
        = m_classifier(data, request.imageUrl().fileName());
    const ImageDecodeRoute route = imageDecodeRouteForClassification(classification);
    qCDebug(kiriviewDecodeLog) << "image decode route"
                               << "generation" << request.id() << "url" << request.imageUrl()
                               << "inputKind" << imageInputKindName(classification.kind)
                               << "handler" << imageDecodeHandlerKindName(route.handlerKind)
                               << "dataSource" << imageDecodeDataSourceName(route.dataSource)
                               << "qtFormat" << qtRasterFormatName(route.qtRasterFormat) << "bytes"
                               << data.size();
    DecodedImageResult result = m_runtime.execute(route, data, request);
    DecodedImage *image = decodedImageResultImage(result);
    if (image != nullptr) {
        EmbeddedMetadata metadata = parseImageEmbeddedMetadata(data);
        if (!metadata.isEmpty()) {
            setDecodedImageEmbeddedMetadata(*image, std::move(metadata));
        }
    }
    return result;
}

DecodedImageResult decodeImageDataWithDefaultRouter(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    static const ImageDecodeRouter router;
    return router.decode(data, request);
}
}
