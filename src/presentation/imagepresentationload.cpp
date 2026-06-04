// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "predecode/predecodecache.h"
#include "presentation/imagepagesurfacecontroller.h"

#include <QImage>
#include <optional>
#include <utility>
#include <variant>

namespace {
KiriView::ImagePresentationLoadResult finishImagePresentation(
    const KiriView::ImagePageSurfaceController &pageSurface)
{
    return KiriView::ImagePresentationLoadResult {
        true,
        pageSurface.imageSize(),
    };
}

KiriView::ImagePresentationLoadResult presentStaticImage(
    KiriView::ImagePageSurfaceController &pageSurface, KiriView::StaticImagePayload staticImage,
    std::optional<KiriView::StaticDisplayImagePayload> displayImage, bool predecodeCacheable,
    const KiriView::ImageDocumentRenderContext &renderContext)
{
    if (displayImage.has_value()) {
        pageSurface.setStaticDisplayImage(
            std::move(*displayImage), predecodeCacheable, renderContext);
    } else {
        pageSurface.setStaticImage(std::move(staticImage), predecodeCacheable, renderContext);
    }
    return finishImagePresentation(pageSurface);
}

KiriView::ImagePresentationLoadResult presentImageFrame(
    KiriView::ImagePageSurfaceController &pageSurface, const QImage &image)
{
    pageSurface.stopAnimation();
    pageSurface.setImage(image, false);
    return finishImagePresentation(pageSurface);
}

KiriView::ImagePresentationLoadPlan staticImagePlan(
    KiriView::StaticImagePayload staticImage, bool predecodeCacheable)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationStaticImageLoad {
        std::move(staticImage),
        std::nullopt,
        predecodeCacheable,
    } };
}

KiriView::ImagePresentationLoadPlan staticDisplayImagePlan(
    KiriView::StaticDisplayImagePayload displayImage, bool predecodeCacheable)
{
    KiriView::StaticImagePayload staticImage = displayImage.compatibilityStaticImage();
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationStaticImageLoad {
        std::move(staticImage),
        std::move(displayImage),
        predecodeCacheable,
    } };
}

KiriView::ImagePresentationLoadPlan framePlan(QImage frame)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationFrameLoad {
        std::move(frame),
    } };
}

KiriView::ImagePresentationLoadPlan animationPlan(
    QImage firstFrame, KiriView::ImageAnimationPlaybackRequest request)
{
    return KiriView::ImagePresentationLoadPlan { KiriView::ImagePresentationAnimationLoad {
        std::move(firstFrame),
        std::move(request),
    } };
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::StaticDecodedImage &decoded,
    KiriView::ImagePresentationAnimationHandling, qsizetype predecodeCacheByteBudget)
{
    KiriView::StaticDisplayImagePayload displayImage = std::move(decoded.displayImage);
    KiriView::StaticImagePayload staticImage = displayImage.compatibilityStaticImage();
    const bool predecodeCacheable
        = KiriView::PredecodeCache::canCacheImage(staticImage, predecodeCacheByteBudget);
    return staticDisplayImagePlan(std::move(displayImage), predecodeCacheable);
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::ApngAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (decoded.firstFrame.isNull()) {
        return {};
    }

    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return animationPlan(std::move(decoded.firstFrame),
        KiriView::apngAnimationPlaybackRequest(std::move(decoded.data)));
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::ReaderAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return animationPlan(std::move(decoded.firstFrame),
        KiriView::readerAnimationPlaybackRequest(
            std::move(decoded.data), std::move(decoded.format)));
}

KiriView::ImagePresentationLoadPlan planDecodedImage(KiriView::HeifSequenceAnimationImage &decoded,
    KiriView::ImagePresentationAnimationHandling animationHandling, qsizetype)
{
    if (animationHandling == KiriView::ImagePresentationAnimationHandling::FirstFrameOnly) {
        return framePlan(std::move(decoded.firstFrame));
    }

    return animationPlan(std::move(decoded.firstFrame),
        KiriView::heifSequenceAnimationPlaybackRequest(std::move(decoded.data)));
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

ImagePresentationLoadPlan planDecodedImagePresentationLoad(DecodedImage image,
    ImagePresentationAnimationHandling animationHandling, qsizetype predecodeCacheByteBudget)
{
    return std::visit(
        [animationHandling, predecodeCacheByteBudget](auto &decoded) {
            return planDecodedImage(decoded, animationHandling, predecodeCacheByteBudget);
        },
        image);
}

ImagePresentationLoadResult executeImagePresentationLoadPlan(
    ImagePageSurfaceController &pageSurface, ImagePresentationLoadPlan plan,
    const ImageDocumentRenderContext &renderContext)
{
    if (std::holds_alternative<std::monostate>(plan.payload)) {
        return {};
    }
    if (auto *staticImage = std::get_if<ImagePresentationStaticImageLoad>(&plan.payload)) {
        return presentStaticImage(pageSurface, std::move(staticImage->staticImage),
            std::move(staticImage->displayImage), staticImage->predecodeCacheable, renderContext);
    }
    if (const auto *frame = std::get_if<ImagePresentationFrameLoad>(&plan.payload)) {
        return presentImageFrame(pageSurface, frame->frame);
    }
    if (auto *animation = std::get_if<ImagePresentationAnimationLoad>(&plan.payload)) {
        ImagePresentationLoadResult result = presentImageFrame(pageSurface, animation->firstFrame);
        pageSurface.startAnimation(std::move(animation->playback));
        return result;
    }

    return {};
}

ImagePresentationLoadResult presentPredecodedImageLoad(ImagePageSurfaceController &pageSurface,
    PredecodedImage image, const ImageDocumentRenderContext &renderContext)
{
    return executeImagePresentationLoadPlan(
        pageSurface, planPredecodedImagePresentationLoad(std::move(image)), renderContext);
}

ImagePresentationLoadResult presentDecodedImageLoad(ImagePageSurfaceController &pageSurface,
    DecodedImage image, ImagePresentationAnimationHandling animationHandling,
    const ImageDocumentRenderContext &renderContext)
{
    return executeImagePresentationLoadPlan(pageSurface,
        planDecodedImagePresentationLoad(
            std::move(image), animationHandling, pageSurface.predecodeCacheByteBudget()),
        renderContext);
}
}
