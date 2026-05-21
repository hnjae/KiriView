// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include "apnganimationreader.h"
#include "heifdecoder.h"
#include "kiriview/src/policy/avifcompat.cxx.h"
#include "localization/imageerrortext.h"
#include "qimagereaderdecoder.h"
#include "rawdecoder.h"
#include "rendering/svgtilesource.h"
#include "rustqtconversion.h"
#include "staticimagedecode.h"

#include <QString>
#include <memory>
#include <optional>
#include <utility>

namespace {
QByteArray avifCompatibleImageData(const QByteArray &data)
{
    return KiriView::Bridge::qtByteArray(
        KiriView::avifDataWithCompatibilityFixes(KiriView::Bridge::rustBytes(data)));
}

class ImageDecodeRouterByteInputs
{
public:
    ImageDecodeRouterByteInputs(const QByteArray &originalData,
        const KiriView::ImageDecodeCompatibleDataTransform &compatibleDataTransform)
        : m_originalData(originalData)
        , m_compatibleDataTransform(compatibleDataTransform)
    {
    }

    const QByteArray &dataFor(KiriView::ImageDecodeDataSource dataSource)
    {
        switch (dataSource) {
        case KiriView::ImageDecodeDataSource::Original:
            return m_originalData;
        case KiriView::ImageDecodeDataSource::AvifCompatible:
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
    const KiriView::ImageDecodeCompatibleDataTransform &m_compatibleDataTransform;
    std::optional<QByteArray> m_compatibleData;
};

KiriView::DecodedImageResult failedReadImageDataResult()
{
    return KiriView::failedDecodedImageResult(
        KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData));
}

KiriView::DecodedImageResult failedImageDataResult(QString errorString)
{
    if (errorString.isEmpty()) {
        return failedReadImageDataResult();
    }
    return KiriView::failedDecodedImageResult(std::move(errorString));
}

KiriView::DecodedImageResult decodeSvgImageData(const KiriView::ImageDecodeRouterInput &input)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(input.data, &errorString);
    if (source == nullptr) {
        return failedImageDataResult(std::move(errorString));
    }

    return KiriView::staticDecodedImageResult(std::move(source), {}, &errorString);
}

KiriView::DecodedImageResult decodeApngImageData(const KiriView::ImageDecodeRouterInput &input)
{
    KiriView::ApngAnimationReader apngReader;
    KiriView::ApngOpenResult apngResult = apngReader.open(input.data);
    if (apngResult.status == KiriView::ApngOpenStatus::NotApng) {
        return KiriView::failedDecodedImageResult(
            KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeApngAnimation));
    }
    if (apngResult.status == KiriView::ApngOpenStatus::Error) {
        return KiriView::failedDecodedImageResult(apngResult.errorString);
    }

    return KiriView::successfulDecodedImageResult(KiriView::ApngAnimationImage {
        std::move(apngResult.firstFrame),
        input.data,
    });
}

KiriView::DecodedImageResult decodeHeifRouterImageData(
    const KiriView::ImageDecodeRouterInput &input)
{
    std::optional<KiriView::DecodedImageResult> result = KiriView::decodeHeifImageData(input.data);
    if (!result.has_value()) {
        return failedReadImageDataResult();
    }
    return std::move(*result);
}

KiriView::DecodedImageResult decodeRawRouterImageData(const KiriView::ImageDecodeRouterInput &input)
{
    return KiriView::decodeRawImageData(input.data, input.request);
}

KiriView::DecodedImageResult decodeQImageReaderRouterImageData(
    const KiriView::ImageDecodeRouterInput &input)
{
    return KiriView::decodeQImageReaderImageData(input.data, input.request, input.qtRasterFormat);
}

KiriView::ImageDecodeRouterHandlers defaultImageDecodeRouterHandlers()
{
    return KiriView::ImageDecodeRouterHandlers {
        decodeSvgImageData,
        decodeApngImageData,
        decodeHeifRouterImageData,
        decodeRawRouterImageData,
        decodeQImageReaderRouterImageData,
    };
}

KiriView::ImageDecodeRouterHandlers withDefaultHandlers(
    KiriView::ImageDecodeRouterHandlers handlers)
{
    const KiriView::ImageDecodeRouterHandlers defaults = defaultImageDecodeRouterHandlers();
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

KiriView::DecodedImageResult dispatchToHandler(const KiriView::ImageDecodeRouterHandler &handler,
    const KiriView::ImageDecodeRouterInput &input)
{
    if (!handler) {
        return failedReadImageDataResult();
    }
    return handler(input);
}

const KiriView::ImageDecodeRouterHandler &emptyHandler()
{
    static const KiriView::ImageDecodeRouterHandler handler;
    return handler;
}

const KiriView::ImageDecodeRouterHandler &handlerForRoute(
    const KiriView::ImageDecodeRouterHandlers &handlers,
    KiriView::ImageDecodeHandlerKind handlerKind)
{
    switch (handlerKind) {
    case KiriView::ImageDecodeHandlerKind::Svg:
        return handlers.svg;
    case KiriView::ImageDecodeHandlerKind::Apng:
        return handlers.apng;
    case KiriView::ImageDecodeHandlerKind::HeifFamily:
        return handlers.heifFamily;
    case KiriView::ImageDecodeHandlerKind::Raw:
        return handlers.raw;
    case KiriView::ImageDecodeHandlerKind::QtRaster:
        return handlers.qtRaster;
    case KiriView::ImageDecodeHandlerKind::None:
        return emptyHandler();
    }

    return emptyHandler();
}
}

namespace KiriView {
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

ImageDecodeRouter::ImageDecodeRouter(ImageDecodeRouterHandlers handlers,
    ImageDecodeInputClassifier classifier,
    ImageDecodeCompatibleDataTransform compatibleDataTransform)
    : m_handlers(withDefaultHandlers(std::move(handlers)))
    , m_classifier(std::move(classifier))
    , m_compatibleDataTransform(std::move(compatibleDataTransform))
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
    if (!route.shouldDecode()) {
        return failedReadImageDataResult();
    }

    ImageDecodeRouterByteInputs byteInputs(data, m_compatibleDataTransform);
    const ImageDecodeRouterInput input {
        byteInputs.dataFor(route.dataSource),
        request,
        route.qtRasterFormat,
    };

    return dispatchToHandler(handlerForRoute(m_handlers, route.handlerKind), input);
}

DecodedImageResult decodeImageDataWithDefaultRouter(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    static const ImageDecodeRouter router;
    return router.decode(data, request);
}
}
