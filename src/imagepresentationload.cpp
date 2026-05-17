// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationload.h"

#include "decodedimagepresentation.h"
#include "imagecontainer.h"
#include "imagepresentationcontroller.h"

#include <QImage>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
void startDecodedAnimation(KiriView::ImagePresentationController &presentation,
    const KiriView::DecodedAnimationImagePresentation &animation)
{
    switch (animation.kind) {
    case KiriView::DecodedImageAnimationKind::Apng:
        presentation.startApngAnimation(
            animation.data, animation.loopCount, animation.firstFrameDelay);
        return;
    case KiriView::DecodedImageAnimationKind::Reader:
        presentation.startAnimation(
            animation.data, animation.format, animation.loopCount, animation.firstFrameDelay);
        return;
    case KiriView::DecodedImageAnimationKind::HeifSequence:
        presentation.startHeifSequenceAnimation(animation.data);
        return;
    }
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

KiriView::ImagePresentationLoadResult presentAnimationImage(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedAnimationImagePresentation animation,
    KiriView::ImagePresentationAnimationHandling animationHandling)
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
    KiriView::DecodedStaticImagePresentation &decoded)
{
    return presentStaticImage(
        presentation, session, std::move(decoded.staticImage), decoded.predecodeCacheable);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &presentation, const KiriView::ImageLoadSession &session,
    KiriView::DecodedAnimationImagePresentation &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling)
{
    return presentAnimationImage(presentation, session, std::move(decoded), animationHandling);
}

KiriView::ImagePresentationLoadResult presentDecodedImagePresentation(
    KiriView::ImagePresentationController &, const KiriView::ImageLoadSession &,
    const KiriView::UnpresentableDecodedImage &)
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
            if constexpr (std::is_same_v<std::decay_t<decltype(decoded)>,
                              DecodedAnimationImagePresentation>) {
                return presentDecodedImagePresentation(
                    presentation, session, decoded, animationHandling);
            } else {
                return presentDecodedImagePresentation(presentation, session, decoded);
            }
        },
        decodedPresentation);
}
}
