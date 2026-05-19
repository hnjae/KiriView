// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONLOAD_H
#define KIRIVIEW_IMAGEPRESENTATIONLOAD_H

#include "decodedimageresult.h"
#include "imageloadtypes.h"
#include "predecodedimage.h"

#include <QByteArray>
#include <QImage>
#include <QSize>

namespace KiriView {
class ImagePresentationController;

enum class ImagePresentationAnimationHandling {
    FirstFrameOnly,
    StartAnimation,
};

struct ImagePresentationLoadResult {
    bool presented = false;
    QSize imageSize;
};

enum class ImagePresentationLoadAction {
    None,
    StaticImage,
    ImageFrame,
    ApngAnimation,
    ReaderAnimation,
    HeifSequenceAnimation,
};

struct ImagePresentationLoadPlan {
    ImagePresentationLoadAction action = ImagePresentationLoadAction::None;
    StaticImagePayload staticImage;
    QImage frame;
    QByteArray animationData;
    QByteArray animationFormat;
    bool predecodeCacheable = false;

    bool hasPresentation() const;
};

ImagePresentationLoadPlan planPredecodedImagePresentationLoad(PredecodedImage image);
ImagePresentationLoadPlan planDecodedImagePresentationLoad(
    DecodedImage image, ImagePresentationAnimationHandling animationHandling);
ImagePresentationLoadResult executeImagePresentationLoadPlan(
    ImagePresentationController &presentation, const ImageLoadSession &session,
    ImagePresentationLoadPlan plan);
ImagePresentationLoadResult presentPredecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, PredecodedImage image);
ImagePresentationLoadResult presentDecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, DecodedImage image,
    ImagePresentationAnimationHandling animationHandling);
}

#endif
