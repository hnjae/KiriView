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
void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::ApngAnimationImage &animation)
{
    presentation.startApngAnimation(animation.data);
}

void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::ReaderAnimationImage &animation)
{
    presentation.startAnimation(animation.data, animation.format);
}

void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::HeifSequenceAnimationImage &animation)
{
    presentation.startHeifSequenceAnimation(animation.data);
}

void prepareImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session)
{
    presentation.prepareImageContainer(KiriView::zoomScopeUrlForLocation(session.location));
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

template <typename AnimationPresentation>
KiriView::ImagePresentationLoadResult presentAnimationImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    AnimationPresentation animation, KiriView::ImagePresentationAnimationHandling animationHandling)
{
    const QImage firstFrame = animation.firstFrame;
    KiriView::ImagePresentationLoadResult result
        = presentImageFrame(presentation, session, firstFrame, false);
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::StartAnimation) {
        startDecodedAnimation(presentation, animation);
    }
    return result;
}

KiriView::ImagePresentationLoadResult presentDecodedImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::StaticDecodedImage &decoded, KiriView::ImagePresentationAnimationHandling)
{
    const bool predecodeCacheable = KiriView::PredecodeCache::canCacheImage(decoded.staticImage);
    return presentStaticImage(
        presentation, session, std::move(decoded.staticImage), predecodeCacheable);
}

KiriView::ImagePresentationLoadResult presentDecodedImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::ApngAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    if (decoded.firstFrame.isNull()) {
        return {};
    }

    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::ReaderAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::HeifSequenceAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}
}

namespace KiriView {
ImagePresentationLoadResult presentPredecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, PredecodedImage image)
{
    return presentStaticImage(presentation, session, std::move(image.staticImage), true);
}

ImagePresentationLoadResult presentDecodedImageLoad(ImagePresentationController &presentation,
    const ImageLoadSession &session, DecodedImage image,
    ImagePresentationAnimationHandling animationHandling)
{
    return std::visit(
        [&presentation, &session, animationHandling](auto &decoded) {
            return presentDecodedImage(presentation, session, decoded, animationHandling);
        },
        image);
}
}
