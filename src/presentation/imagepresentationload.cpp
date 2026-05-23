// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "location/imagedocumentlocation.h"
#include "predecode/predecodecache.h"
#include "presentation/imagepresentationcontroller.h"

#include <QImage>
#include <utility>
#include <variant>

namespace {
void prepareImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session)
{
    presentation.prepareImageContainer(KiriView::zoomScopeUrlForLocation(session.location()));
}

KiriView::ImagePresentationLoadResult finishImagePresentation(
    KiriView::ImagePresentationController &presentation)
{
    presentation.updateRenderContext();
    return KiriView::ImagePresentationLoadResult {
        true,
        presentation.imageSize(),
    };
}

KiriView::ImagePresentationLoadResult presentStaticImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::StaticImagePayload staticImage, bool predecodeCacheable)
{
    prepareImagePresentation(presentation, session);
    presentation.setStaticImage(std::move(staticImage), predecodeCacheable);
    return finishImagePresentation(presentation);
}

KiriView::ImagePresentationLoadResult presentImageFrame(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    const QImage &image)
{
    prepareImagePresentation(presentation, session);
    presentation.stopAnimation();
    presentation.setImage(image, false);
    return finishImagePresentation(presentation);
}

KiriView::ImagePresentationLoadPlan staticImagePlan(
    KiriView::StaticImagePayload staticImage, bool predecodeCacheable)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationStaticImageLoad {
        std::move(staticImage),
        predecodeCacheable,
    } };
}

KiriView::ImagePresentationLoadPlan framePlan(QImage frame)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationFrameLoad {
        std::move(frame),
    } };
}

KiriView::ImagePresentationLoadPlan apngAnimationPlan(QImage firstFrame, QByteArray data)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationApngAnimationLoad {
        std::move(firstFrame),
        std::move(data),
    } };
}

KiriView::ImagePresentationLoadPlan readerAnimationPlan(
    QImage firstFrame, QByteArray data, QByteArray format)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationReaderAnimationLoad {
        std::move(firstFrame),
        std::move(data),
        std::move(format),
    } };
}

KiriView::ImagePresentationLoadPlan heifSequenceAnimationPlan(QImage firstFrame, QByteArray data)
{
    return KiriView::ImagePresentationLoadPlan {
        KiriView::ImagePresentationHeifSequenceAnimationLoad {
            std::move(firstFrame),
            std::move(data),
        },
    };
}

KiriView::ImagePresentationLoadPlan planDecodedImage(
    KiriView::StaticDecodedImage &decoded, KiriView::ImagePresentationAnimationHandling)
{
    const bool predecodeCacheable = KiriView::PredecodeCache::canCacheImage(decoded.staticImage);
    return staticImagePlan(std::move(decoded.staticImage), predecodeCacheable);
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::ApngAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    if (decoded.firstFrame.isNull()) {
        return {};
    }

    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return apngAnimationPlan(std::move(decoded.firstFrame), std::move(decoded.data));
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::ReaderAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return readerAnimationPlan(
        std::move(decoded.firstFrame), std::move(decoded.data), std::move(decoded.format));
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::HeifSequenceAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return heifSequenceAnimationPlan(std::move(decoded.firstFrame), std::move(decoded.data));
}
}

namespace KiriView {
bool ImagePresentationLoadPlan::hasPresentation() const
{
    return !std::holds_alternative<std::monostate>(payload);
}

ImagePresentationLoadPlan planPredecodedImagePresentationLoad(PredecodedImage image)
{
    return staticImagePlan(std::move(image.staticImage), true);
}

ImagePresentationLoadPlan planDecodedImagePresentationLoad(
    DecodedImage image, ImagePresentationAnimationHandling animationHandling)
{
    return std::visit(
        [animationHandling](auto &decoded) { return planDecodedImage(decoded, animationHandling); },
        image);
}

ImagePresentationLoadResult executeImagePresentationLoadPlan(
    ImagePresentationController &presentation, const ImageLoadSession &session,
    ImagePresentationLoadPlan plan)
{
    if (std::holds_alternative<std::monostate>(plan.payload)) {
        return {};
    }
    if (auto *staticImage = std::get_if<ImagePresentationStaticImageLoad>(&plan.payload)) {
        return presentStaticImage(presentation, session, std::move(staticImage->staticImage),
            staticImage->predecodeCacheable);
    }
    if (const auto *frame = std::get_if<ImagePresentationFrameLoad>(&plan.payload)) {
        return presentImageFrame(presentation, session, frame->frame);
    }
    if (auto *animation = std::get_if<ImagePresentationApngAnimationLoad>(&plan.payload)) {
        ImagePresentationLoadResult result
            = presentImageFrame(presentation, session, animation->firstFrame);
        presentation.startApngAnimation(animation->data);
        return result;
    }
    if (auto *animation = std::get_if<ImagePresentationReaderAnimationLoad>(&plan.payload)) {
        ImagePresentationLoadResult result
            = presentImageFrame(presentation, session, animation->firstFrame);
        presentation.startAnimation(animation->data, animation->format);
        return result;
    }
    if (auto *animation = std::get_if<ImagePresentationHeifSequenceAnimationLoad>(&plan.payload)) {
        ImagePresentationLoadResult result
            = presentImageFrame(presentation, session, animation->firstFrame);
        presentation.startHeifSequenceAnimation(animation->data);
        return result;
    }

    return {};
}

ImagePresentationLoadResult presentPredecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, PredecodedImage image)
{
    return executeImagePresentationLoadPlan(
        presentation, session, planPredecodedImagePresentationLoad(std::move(image)));
}

ImagePresentationLoadResult presentDecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, DecodedImage image,
    ImagePresentationAnimationHandling animationHandling)
{
    return executeImagePresentationLoadPlan(presentation, session,
        planDecodedImagePresentationLoad(std::move(image), animationHandling));
}
}
