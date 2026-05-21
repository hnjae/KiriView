// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEPIPELINE_H
#define KIRIVIEW_IMAGEDECODEPIPELINE_H

#include "decodedimageresult.h"
#include "imagedecoderequest.h"
#include "imageinputclassification.h"

#include <QByteArray>
#include <functional>

namespace KiriView {
struct ImageDecodeRouterInput {
    const QByteArray &data;
    const ImageDecodeRequest &request;
    QtRasterFormat qtRasterFormat = QtRasterFormat::None;
};

enum class ImageDecodeHandlerKind {
    None,
    Svg,
    Apng,
    HeifFamily,
    Raw,
    QtRaster,
};

struct ImageDecodeRoute {
    ImageDecodeHandlerKind handlerKind = ImageDecodeHandlerKind::None;
    ImageDecodeDataSource dataSource = ImageDecodeDataSource::Original;
    QtRasterFormat qtRasterFormat = QtRasterFormat::None;

    bool shouldDecode() const { return handlerKind != ImageDecodeHandlerKind::None; }
};

using ImageDecodeRouterHandler = std::function<DecodedImageResult(const ImageDecodeRouterInput &)>;

struct ImageDecodeRouterHandlers {
    ImageDecodeRouterHandler svg;
    ImageDecodeRouterHandler apng;
    ImageDecodeRouterHandler heifFamily;
    ImageDecodeRouterHandler raw;
    ImageDecodeRouterHandler qtRaster;
};

using ImageDecodeInputClassifier
    = std::function<ImageInputClassification(const QByteArray &, const QString &)>;
using ImageDecodeCompatibleDataTransform = std::function<QByteArray(const QByteArray &)>;

ImageDecodeRoute imageDecodeRouteForClassification(ImageInputClassification classification);

class ImageDecodeRouter final
{
public:
    explicit ImageDecodeRouter(ImageDecodeRouterHandlers handlers = {},
        ImageDecodeInputClassifier classifier = {},
        ImageDecodeCompatibleDataTransform compatibleDataTransform = {});

    DecodedImageResult decode(const QByteArray &data, const ImageDecodeRequest &request) const;

private:
    ImageDecodeRouterHandlers m_handlers;
    ImageDecodeInputClassifier m_classifier;
    ImageDecodeCompatibleDataTransform m_compatibleDataTransform;
};

DecodedImageResult decodeImageDataWithDefaultRouter(
    const QByteArray &data, const ImageDecodeRequest &request);
}

#endif
