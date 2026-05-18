// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationload.h"

#include "decodedimagepresentation.h"
#include "imagecontainer.h"
#include "imagepresentationcontroller.h"

#include <QImage>
#include <utility>
#include <variant>

namespace {
void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::DecodedApngAnimationPresentation &animation)
{
    presentation.startApngAnimation(animation.data);
}

void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::DecodedReaderAnimationPresentation &animation)
{
    presentation.startAnimation(animation.data, animation.format);
}

void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::DecodedHeifSequenceAnimationPresentation &animation)
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

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedStaticImagePresentation &decoded, KiriView::ImagePresentationAnimationHandling)
{
    return presentStaticImage(
        presentation, session, std::move(decoded.staticImage), decoded.predecodeCacheable);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedApngAnimationPresentation &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedReaderAnimationPresentation &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedHeifSequenceAnimationPresentation &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &, const KiriView::ImageLoadSession &,
    const KiriView::UnpresentableDecodedImage &, KiriView::ImagePresentationAnimationHandling)
{
    return {};
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
    DecodedImagePresentation decodedPresentation
        = decodedImagePresentationForImage(std::move(image));
    return std::visit(
        [&presentation, &session, animationHandling](auto &decoded) {
            return presentDecodedImagePresentation(
                presentation, session, decoded, animationHandling);
        },
        decodedPresentation);
}
}
