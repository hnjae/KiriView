// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include "apnganimationreader.h"
#include "heifdecoder.h"
#include "kiriview/src/avifcompat.cxx.h"
#include "qimagereaderdecoder.h"
#include "rawdecoder.h"
#include "rustqtconversion.h"
#include "staticimagedecode.h"
#include "svgtilesource.h"

#include <QString>
#include <memory>
#include <utility>

namespace {
QByteArray avifCompatibleImageData(const QByteArray &data)
{
    return KiriView::Bridge::qtByteArray(
        KiriView::avifDataWithCompatibilityFixes(KiriView::Bridge::rustBytes(data)));
}

std::optional<KiriView::DecodedImageResult> decodeSvgImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(input.originalData, &errorString);
    if (source == nullptr) {
        return std::nullopt;
    }

    return KiriView::staticDecodedImageResult(std::move(source), {}, &errorString);
}

std::optional<KiriView::DecodedImageResult> decodeApngImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    KiriView::ApngAnimationReader apngReader;
    KiriView::ApngOpenResult apngResult = apngReader.open(input.originalData);
    if (apngResult.status == KiriView::ApngOpenStatus::NotApng) {
        return std::nullopt;
    }
    if (apngResult.status == KiriView::ApngOpenStatus::Error) {
        return KiriView::failedDecodedImageResult(apngResult.errorString);
    }

    return KiriView::successfulDecodedImageResult(KiriView::ApngAnimationImage {
        std::move(apngResult.firstFrame),
        input.originalData,
    });
}

std::optional<KiriView::DecodedImageResult> decodeHeifPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeHeifImageData(input.compatibleData);
}

std::optional<KiriView::DecodedImageResult> decodeRawPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeRawImageData(input.compatibleData, input.request);
}

std::optional<KiriView::DecodedImageResult> decodeQImageReaderPipelineImageData(
    const KiriView::ImageDecodePipelineInput &input)
{
    return KiriView::decodeQImageReaderImageData(input.compatibleData, input.request);
}

std::vector<KiriView::ImageDecodePipelineStage> defaultImageDecodeStages()
{
    return {
        decodeSvgImageData,
        decodeApngImageData,
        decodeHeifPipelineImageData,
        decodeRawPipelineImageData,
        decodeQImageReaderPipelineImageData,
    };
}
}

namespace KiriView {
ImageDecodePipeline::ImageDecodePipeline(std::vector<ImageDecodePipelineStage> stages)
    : m_stages(std::move(stages))
{
}

DecodedImageResult ImageDecodePipeline::decode(
    const QByteArray &data, const ImageDecodeRequest &request) const
{
    const QByteArray compatibleData = avifCompatibleImageData(data);
    const ImageDecodePipelineInput input {
        data,
        compatibleData,
        request,
    };

    for (const ImageDecodePipelineStage &stage : m_stages) {
        if (!stage) {
            continue;
        }

        std::optional<DecodedImageResult> result = stage(input);
        if (result.has_value()) {
            return std::move(*result);
        }
    }

    return failedDecodedImageResult(QStringLiteral("Could not read the selected image data."));
}

DecodedImageResult decodeImageDataWithDefaultPipeline(
    const QByteArray &data, const ImageDecodeRequest &request)
{
    static const ImageDecodePipeline pipeline(defaultImageDecodeStages());
    return pipeline.decode(data, request);
}
}
