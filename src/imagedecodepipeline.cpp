// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include "apnganimationreader.h"
#include "heifdecoder.h"
#include "imageerrortext.h"
#include "kiriview/src/avifcompat.cxx.h"
#include "qimagereaderdecoder.h"
#include "rawdecoder.h"
#include "rustqtconversion.h"
#include "staticimagedecode.h"
#include "svgtilesource.h"

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

class ImageDecodePipelineByteInputs
{
public:
    ImageDecodePipelineByteInputs(const QByteArray &originalData,
        const KiriView::ImageDecodeCompatibleDataTransform &compatibleDataTransform)
        : m_originalData(originalData)
        , m_compatibleDataTransform(compatibleDataTransform)
    {
    }

    const QByteArray &dataFor(KiriView::ImageDecodePipelineDataSource dataSource)
    {
        switch (dataSource) {
        case KiriView::ImageDecodePipelineDataSource::Original:
            return m_originalData;
        case KiriView::ImageDecodePipelineDataSource::AvifCompatible:
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

KiriView::ImageDecodePipelineStageResult optionalDecodedStageResult(
    std::optional<KiriView::DecodedImageResult> result)
{
    if (!result.has_value()) {
        return KiriView::ImageDecodePipelineStageResult::unsupported();
    }

    return KiriView::ImageDecodePipelineStageResult::decoded(std::move(*result));
}

KiriView::ImageDecodePipelineStageResult decodeSvgImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(input.data, &errorString);
    if (source == nullptr) {
        return KiriView::ImageDecodePipelineStageResult::unsupported();
    }

    return KiriView::ImageDecodePipelineStageResult::decoded(
        KiriView::staticDecodedImageResult(std::move(source), {}, &errorString));
}

KiriView::ImageDecodePipelineStageResult decodeApngImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    KiriView::ApngAnimationReader apngReader;
    KiriView::ApngOpenResult apngResult = apngReader.open(input.data);
    if (apngResult.status == KiriView::ApngOpenStatus::NotApng) {
        return KiriView::ImageDecodePipelineStageResult::unsupported();
    }
    if (apngResult.status == KiriView::ApngOpenStatus::Error) {
        return KiriView::ImageDecodePipelineStageResult::decoded(
            KiriView::failedDecodedImageResult(apngResult.errorString));
    }

    return KiriView::ImageDecodePipelineStageResult::decoded(
        KiriView::successfulDecodedImageResult(KiriView::ApngAnimationImage {
            std::move(apngResult.firstFrame),
            input.data,
        }));
}

KiriView::ImageDecodePipelineStageResult decodeHeifPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return optionalDecodedStageResult(KiriView::decodeHeifImageData(input.data));
}

KiriView::ImageDecodePipelineStageResult decodeRawPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return optionalDecodedStageResult(KiriView::decodeRawImageData(input.data, input.request));
}

KiriView::ImageDecodePipelineStageResult decodeQImageReaderPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::ImageDecodePipelineStageResult::decoded(
        KiriView::decodeQImageReaderImageData(input.data, input.request));
}

std::vector<KiriView::ImageDecodePipelineStage> defaultImageDecodeStages()
{
    using DataSource = KiriView::ImageDecodePipelineDataSource;
    return {
        { DataSource::Original, decodeSvgImageData },
        { DataSource::Original, decodeApngImageData },
        { DataSource::AvifCompatible, decodeHeifPipelineImageData },
        { DataSource::Original, decodeRawPipelineImageData },
        { DataSource::AvifCompatible, decodeQImageReaderPipelineImageData },
    };
}
}

namespace KiriView {
ImageDecodePipelineStageResult ImageDecodePipelineStageResult::unsupported()
{
    return ImageDecodePipelineStageResult(std::nullopt);
}

ImageDecodePipelineStageResult ImageDecodePipelineStageResult::decoded(DecodedImageResult result)
{
    return ImageDecodePipelineStageResult(std::optional<DecodedImageResult>(std::move(result)));
}

bool ImageDecodePipelineStageResult::handled() const { return m_result.has_value(); }

std::optional<DecodedImageResult> ImageDecodePipelineStageResult::takeDecodedResult() &&
{
    return std::move(m_result);
}

ImageDecodePipelineStageResult::ImageDecodePipelineStageResult(
    std::optional<DecodedImageResult> result)
    : m_result(std::move(result))
{
}

ImageDecodePipeline::ImageDecodePipeline(std::vector<ImageDecodePipelineStage> stages,
    ImageDecodeCompatibleDataTransform compatibleDataTransform)
    : m_stages(std::move(stages))
    , m_compatibleDataTransform(std::move(compatibleDataTransform))
{
}

DecodedImageResult ImageDecodePipeline::decode(
    const QByteArray &data, const ImageDecodeRequest &request) const
{
    ImageDecodePipelineByteInputs byteInputs(data, m_compatibleDataTransform);

    for (const ImageDecodePipelineStage &stage : m_stages) {
        if (!stage.handler) {
            continue;
        }

        const ImageDecodePipelineInput input {
            byteInputs.dataFor(stage.dataSource),
            request,
        };
        ImageDecodePipelineStageResult result = stage.handler(input);
        if (result.handled()) {
            std::optional<DecodedImageResult> decodedResult = std::move(result).takeDecodedResult();
            return std::move(*decodedResult);
        }
    }

    return failedDecodedImageResult(imageErrorText(ImageErrorTextId::ReadImageData));
}

DecodedImageResult decodeImageDataWithDefaultPipeline(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    static const ImageDecodePipeline pipeline(defaultImageDecodeStages());
    return pipeline.decode(data, request);
}
}
