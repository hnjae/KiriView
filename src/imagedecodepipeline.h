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
    const QByteArray &originalData;
    const QByteArray &compatibleData;
    const ImageDecodeRequest &request;
};

using ImageDecodePipelineStage
    = std::function<std::optional<DecodedImageResult>(const ImageDecodePipelineInput &)>;

class ImageDecodePipeline final
{
public:
    explicit ImageDecodePipeline(std::vector<ImageDecodePipelineStage> stages);

    DecodedImageResult decode(const QByteArray &data, const ImageDecodeRequest &request) const;

private:
    std::vector<ImageDecodePipelineStage> m_stages;
};

DecodedImageResult decodeImageDataWithDefaultPipeline(
    const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
