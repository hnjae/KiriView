// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "predecode/predecodecache.h"
#include "presentation/imagepagesurfacecontroller.h"

#include <QImage>
#include <utility>
#include <variant>

namespace {
kiriview::ImagePresentationLoadResult finishImagePresentation(
    const kiriview::ImagePageSurfaceController& pageSurface)
{
    return kiriview::ImagePresentationLoadResult {
        true,
        pageSurface.imageSize(),
    };
}

kiriview::ImagePresentationLoadResult presentStaticImage(
    kiriview::ImagePageSurfaceController& pageSurface,
    kiriview::StaticDisplayImagePayload displayImage, bool predecodeCacheable,
    const kiriview::ImageDocumentRenderContext& renderContext)
{
    pageSurface.setStaticDisplayImage(std::move(displayImage), predecodeCacheable, renderContext);
    return finishImagePresentation(pageSurface);
}

kiriview::ImagePresentationLoadResult presentImageFrame(
    kiriview::ImagePageSurfaceController& pageSurface, const QImage& image,
    const QString& sourceIdentity)
{
    pageSurface.stopAnimation();
    pageSurface.setAnimationFrame(image, sourceIdentity);
    return finishImagePresentation(pageSurface);
}

kiriview::ImagePresentationLoadPlan staticDisplayImagePlan(
    kiriview::StaticDisplayImagePayload displayImage, bool predecodeCacheable)
{
    return kiriview::ImagePresentationLoadPlan { kiriview::ImagePresentationStaticImageLoad {
        std::move(displayImage),
        predecodeCacheable,
    } };
}

kiriview::ImagePresentationLoadPlan framePlan(QImage frame, QString sourceIdentity)
{
    return kiriview::ImagePresentationLoadPlan { kiriview::ImagePresentationFrameLoad {
        std::move(frame),
        std::move(sourceIdentity),
    } };
}

kiriview::ImagePresentationLoadPlan animationPlan(
    QImage firstFrame, kiriview::ImageAnimationPlaybackRequest request, QString sourceIdentity)
{
    return kiriview::ImagePresentationLoadPlan { kiriview::ImagePresentationAnimationLoad {
        std::move(firstFrame),
        std::move(request),
        std::move(sourceIdentity),
    } };
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::StaticDecodedImage& decoded,
    kiriview::ImagePresentationAnimationHandling, qsizetype predecodeCacheByteBudget)
{
    kiriview::StaticDisplayImagePayload displayImage = std::move(decoded.displayImage);
    const bool predecodeCacheable
        = kiriview::PredecodeCache::canCacheImage(displayImage, predecodeCacheByteBudget);
    return staticDisplayImagePlan(std::move(displayImage), predecodeCacheable);
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::ApngAnimationImage& decoded,
    kiriview::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (decoded.firstFrame.isNull()) {
        return {};
    }

    if (animationHandling == kiriview::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame), std::move(decoded.sourceIdentity));
    }

    return animationPlan(std::move(decoded.firstFrame),
        kiriview::apngAnimationPlaybackRequest(std::move(decoded.data)),
        std::move(decoded.sourceIdentity));
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::ReaderAnimationImage& decoded,
    kiriview::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == kiriview::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame), std::move(decoded.sourceIdentity));
    }

    return animationPlan(std::move(decoded.firstFrame),
        kiriview::readerAnimationPlaybackRequest(
            std::move(decoded.data), std::move(decoded.format)),
        std::move(decoded.sourceIdentity));
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::WebPAnimationImage& decoded,
    kiriview::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == kiriview::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame), std::move(decoded.sourceIdentity));
    }

    return animationPlan(std::move(decoded.firstFrame),
        kiriview::webpAnimationPlaybackRequest(std::move(decoded.data)),
        std::move(decoded.sourceIdentity));
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::JxlAnimationImage& decoded,
    kiriview::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == kiriview::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame), std::move(decoded.sourceIdentity));
    }

    return animationPlan(std::move(decoded.firstFrame),
        kiriview::jxlAnimationPlaybackRequest(std::move(decoded.data)),
        std::move(decoded.sourceIdentity));
}

kiriview::ImagePresentationLoadPlan planDecodedImage(kiriview::HeifSequenceAnimationImage& decoded,
    kiriview::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == kiriview::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame), std::move(decoded.sourceIdentity));
    }

    return animationPlan(std::move(decoded.firstFrame),
        kiriview::heifSequenceAnimationPlaybackRequest(std::move(decoded.data)),
        std::move(decoded.sourceIdentity));
}
}

namespace kiriview {
bool ImagePresentationLoadPlan::hasPresentation() const
{
    return !std::holds_alternative<std::monostate>(payload);
}

ImagePresentationLoadPlan planPredecodedImagePresentationLoad(PredecodedImage image)
{
    return staticDisplayImagePlan(std::move(image.displayImage), true);
}

ImagePresentationLoadPlan planDecodedImagePresentationLoad(DecodedImage image,
    ImagePresentationAnimationHandling animationHandling, qsizetype predecodeCacheByteBudget)
{
    return std::visit(
        [animationHandling, predecodeCacheByteBudget](auto& decoded) {
            return planDecodedImage(decoded, animationHandling, predecodeCacheByteBudget);
        },
        image);
}

ImagePresentationLoadResult executeImagePresentationLoadPlan(
    ImagePageSurfaceController& pageSurface, ImagePresentationLoadPlan plan,
    const ImageDocumentRenderContext& renderContext)
{
    if (std::holds_alternative<std::monostate>(plan.payload)) {
        return {};
    }
    if (auto* staticImage = std::get_if<ImagePresentationStaticImageLoad>(&plan.payload)) {
        return presentStaticImage(pageSurface, std::move(staticImage->displayImage),
            staticImage->predecodeCacheable, renderContext);
    }
    if (const auto* frame = std::get_if<ImagePresentationFrameLoad>(&plan.payload)) {
        return presentImageFrame(pageSurface, frame->frame, frame->sourceIdentity);
    }
    if (auto* animation = std::get_if<ImagePresentationAnimationLoad>(&plan.payload)) {
        ImagePresentationLoadResult result
            = presentImageFrame(pageSurface, animation->firstFrame, animation->sourceIdentity);
        pageSurface.startAnimation(std::move(animation->playback));
        return result;
    }

    return {};
}

ImagePresentationLoadResult presentPredecodedImageLoad(ImagePageSurfaceController& pageSurface,
    PredecodedImage image, const ImageDocumentRenderContext& renderContext)
{
    return executeImagePresentationLoadPlan(
        pageSurface, planPredecodedImagePresentationLoad(std::move(image)), renderContext);
}

ImagePresentationLoadResult presentDecodedImageLoad(ImagePageSurfaceController& pageSurface,
    DecodedImage image, ImagePresentationAnimationHandling animationHandling,
    const ImageDocumentRenderContext& renderContext)
{
    return executeImagePresentationLoadPlan(pageSurface,
        planDecodedImagePresentationLoad(
            std::move(image), animationHandling, pageSurface.predecodeCacheByteBudget()),
        renderContext);
}
}
