// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEPIPELINE_H
#define KIRIVIEW_IMAGEDECODEPIPELINE_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"

#include <QByteArray>
#include <functional>
#include <optional>
#include <vector>

namespace KiriView {
struct ImageDecodePipelineInput {
    const QByteArray &data;
    const ImageDecodeRequest &request;
};

enum class ImageDecodePipelineDataSource {
    Original,
    AvifCompatible,
};

class ImageDecodePipelineStageResult final
{
public:
    static ImageDecodePipelineStageResult unsupported();
    static ImageDecodePipelineStageResult decoded(DecodedImageResult result);

    bool handled() const;
    std::optional<DecodedImageResult> takeDecodedResult() &&;

private:
    explicit ImageDecodePipelineStageResult(std::optional<DecodedImageResult> result);

    std::optional<DecodedImageResult> m_result;
};

using ImageDecodePipelineHandler
    = std::function<ImageDecodePipelineStageResult(const ImageDecodePipelineInput &)>;

struct ImageDecodePipelineStage {
    ImageDecodePipelineDataSource dataSource = ImageDecodePipelineDataSource::Original;
    ImageDecodePipelineHandler handler;
};

using ImageDecodeCompatibleDataTransform = std::function<QByteArray(const QByteArray &)>;

class ImageDecodePipeline final
{
public:
    explicit ImageDecodePipeline(std::vector<ImageDecodePipelineStage> stages,
        ImageDecodeCompatibleDataTransform compatibleDataTransform = {});

    DecodedImageResult decode(const QByteArray &data, const ImageDecodeRequest &request) const;

private:
    std::vector<ImageDecodePipelineStage> m_stages;
    ImageDecodeCompatibleDataTransform m_compatibleDataTransform;
};

DecodedImageResult decodeImageDataWithDefaultPipeline(
    const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
