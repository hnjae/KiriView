// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONLOAD_H
#define KIRIVIEW_IMAGEPRESENTATIONLOAD_H

#include "decoding/decodedimageresult.h"
#include "document/imageloadtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imageanimationplaybacksource.h"
#include "rendering/imagerendercontext.h"

#include <QImage>
#include <QSize>
#include <QString>
#include <QtGlobal>
#include <variant>

namespace KiriView {
class ImagePageSurfaceController;

enum class ImagePresentationAnimationHandling {
    FirstFrameOnly,
    StartAnimation,
};

struct ImagePresentationLoadResult {
    bool presented = false;
    QSize imageSize;
};

struct ImagePresentationStaticImageLoad {
    StaticDisplayImagePayload displayImage;
    bool predecodeCacheable = false;
};

struct ImagePresentationFrameLoad {
    QImage frame;
    QString sourceIdentity;
};

struct ImagePresentationAnimationLoad {
    QImage firstFrame;
    ImageAnimationPlaybackRequest playback;
    QString sourceIdentity;
};

struct ImagePresentationLoadPlan {
    using Payload = std::variant<std::monostate, ImagePresentationStaticImageLoad,
        ImagePresentationFrameLoad, ImagePresentationAnimationLoad>;

    Payload payload;

    bool hasPresentation() const;
};

ImagePresentationLoadPlan planPredecodedImagePresentationLoad(PredecodedImage image);
ImagePresentationLoadPlan planDecodedImagePresentationLoad(DecodedImage image,
    ImagePresentationAnimationHandling animationHandling, qsizetype predecodeCacheByteBudget);
ImagePresentationLoadResult executeImagePresentationLoadPlan(
    ImagePageSurfaceController &pageSurface, ImagePresentationLoadPlan plan,
    const ImageDocumentRenderContext &renderContext);
ImagePresentationLoadResult presentPredecodedImageLoad(ImagePageSurfaceController &pageSurface,
    PredecodedImage image, const ImageDocumentRenderContext &renderContext);
ImagePresentationLoadResult presentDecodedImageLoad(ImagePageSurfaceController &pageSurface,
    DecodedImage image, ImagePresentationAnimationHandling animationHandling,
    const ImageDocumentRenderContext &renderContext);
}

#endif
