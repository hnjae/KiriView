// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationload.h"

#include "imagecontainer.h"
#include "imagepresentationcontroller.h"
#include "predecodecache.h"

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
    const QImage &image, bool predecodeCacheable)
{
    prepareImagePresentation(presentation, session);
    presentation.stopAnimation();
    presentation.setImage(image, predecodeCacheable);
    return finishImagePresentation(presentation);
}

KiriView::ImagePresentationLoadPlan staticImagePlan(
    KiriView::StaticImagePayload staticImage, bool predecodeCacheable)
{
    return KiriView::ImagePresentationLoadPlan {
        KiriView::ImagePresentationLoadAction::StaticImage,
        std::move(staticImage),
        {},
        {},
        {},
        predecodeCacheable,
    };
}

KiriView::ImagePresentationLoadPlan framePlan(QImage frame)
{
    return KiriView::ImagePresentationLoadPlan {
        KiriView::ImagePresentationLoadAction::ImageFrame,
        {},
        std::move(frame),
        {},
        {},
        false,
    };
}

KiriView::ImagePresentationLoadPlan animationPlan(KiriView::ImagePresentationLoadAction action,
    QImage firstFrame, QByteArray data, QByteArray format = {})
{
    return KiriView::ImagePresentationLoadPlan {
        action,
        {},
        std::move(firstFrame),
        std::move(data),
        std::move(format),
        false,
    };
}

KiriView::ImagePresentationLoadPlan planAnimationImage(KiriView::ImagePresentationLoadAction action,
    QImage firstFrame, QByteArray data, QByteArray format,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(firstFrame));
    }

    return animationPlan(action, std::move(firstFrame), std::move(data), std::move(format));
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

    return planAnimationImage(KiriView::ImagePresentationLoadAction::ApngAnimation,
        std::move(decoded.firstFrame), std::move(decoded.data), {}, animationHandling);
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::ReaderAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return planAnimationImage(KiriView::ImagePresentationLoadAction::ReaderAnimation,
        std::move(decoded.firstFrame), std::move(decoded.data), std::move(decoded.format),
        animationHandling);
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::HeifSequenceAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return planAnimationImage(KiriView::ImagePresentationLoadAction::HeifSequenceAnimation,
        std::move(decoded.firstFrame), std::move(decoded.data), {}, animationHandling);
}

void startPlannedAnimation(
    KiriView::ImagePresentationController &presentation, KiriView::ImagePresentationLoadPlan &plan)
{
    switch (plan.action) {
    case KiriView::ImagePresentationLoadAction::ApngAnimation:
        presentation.startApngAnimation(plan.animationData);
        return;
    case KiriView::ImagePresentationLoadAction::ReaderAnimation:
        presentation.startAnimation(plan.animationData, plan.animationFormat);
        return;
    case KiriView::ImagePresentationLoadAction::HeifSequenceAnimation:
        presentation.startHeifSequenceAnimation(plan.animationData);
        return;
    case KiriView::ImagePresentationLoadAction::None:
    case KiriView::ImagePresentationLoadAction::StaticImage:
    case KiriView::ImagePresentationLoadAction::ImageFrame:
        return;
    }
}
}

namespace KiriView {
bool ImagePresentationLoadPlan::hasPresentation() const
{
    return action != ImagePresentationLoadAction::None;
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
    switch (plan.action) {
    case ImagePresentationLoadAction::None:
        return {};
    case ImagePresentationLoadAction::StaticImage:
        return presentStaticImage(
            presentation, session, std::move(plan.staticImage), plan.predecodeCacheable);
    case ImagePresentationLoadAction::ImageFrame:
        return presentImageFrame(presentation, session, plan.frame, plan.predecodeCacheable);
    case ImagePresentationLoadAction::ApngAnimation:
    case ImagePresentationLoadAction::ReaderAnimation:
    case ImagePresentationLoadAction::HeifSequenceAnimation: {
        ImagePresentationLoadResult result
            = presentImageFrame(presentation, session, plan.frame, false);
        startPlannedAnimation(presentation, plan);
        return result;
    }
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
