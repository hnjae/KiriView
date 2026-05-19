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

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(input.data, &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    return KiriView::staticDecodedImageResult(std::move(source), {}, &errorString);
}

std::optional<KiriView::DecodedImageResult> decodeApngImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    KiriView::ApngAnimationReader apngReader;
    KiriView::ApngOpenResult apngResult = apngReader.open(input.data);
    if (apngResult.status == KiriView::ApngOpenStatus::NotApng) {
        return std::nullopt;
    }
    if (apngResult.status == KiriView::ApngOpenStatus::Error) {
        return KiriView::failedDecodedImageResult(apngResult.errorString);
    }

    return KiriView::successfulDecodedImageResult(KiriView::ApngAnimationImage {
        std::move(apngResult.firstFrame),
        input.data,
    });
}

std::optional<KiriView::DecodedImageResult> decodeHeifPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeHeifImageData(input.data);
}

std::optional<KiriView::DecodedImageResult> decodeRawPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeRawImageData(input.data, input.request);
}

std::optional<KiriView::DecodedImageResult> decodeQImageReaderPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeQImageReaderImageData(input.data, input.request);
}

std::vector<KiriView::ImageDecodePipelineStage> defaultImageDecodeStages()
{
    using DataSource = KiriView::ImageDecodePipelineDataSource;
    return {
        { DataSource::Original, decodeSvgImageData },
        { DataSource::Original, decodeApngImageData },
        { DataSource::AvifCompatible, decodeHeifPipelineImageData },
        { DataSource::AvifCompatible, decodeRawPipelineImageData },
        { DataSource::AvifCompatible, decodeQImageReaderPipelineImageData },
    };
}
}

namespace KiriView {
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
        std::optional<DecodedImageResult> result = stage.handler(input);
        if (result.has_value()) {
            return std::move(*result);
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
