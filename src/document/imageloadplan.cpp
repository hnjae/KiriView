// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadplan.h"

#include "location/imagedocumentlocation.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
using KiriView::directOpenImagePageScopeLocationForLocalUrl;
using KiriView::ImageLoadStartEffect;
using KiriView::imagePageScopeContainsUrl;
using KiriView::ImagePageScopeLoadEffect;
using KiriView::ImagePageScopeLocation;
using KiriView::imagePageScopeLocationForLocalArchiveUrl;

std::optional<ImagePageScopeLocation> containerImagePageScopeForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    if (!request.isContainerNavigation()) {
        return std::nullopt;
    }

    const std::optional<ImagePageScopeLocation> containerImagePageScope
        = imagePageScopeLocationForLocalArchiveUrl(request.containerNavigationUrl());
    if (containerImagePageScope.has_value()
        && imagePageScopeContainsUrl(*containerImagePageScope, request.sourceUrl())) {
        return containerImagePageScope;
    }

    return std::nullopt;
}

ImageLoadStartEffect imageLoadStartEffectForPageScopeEffect(ImagePageScopeLoadEffect effect)
{
    switch (effect) {
    case ImagePageScopeLoadEffect::ReadImage:
        return ImageLoadStartEffect::DecodeImage;
    case ImagePageScopeLoadEffect::LoadImageCandidates:
        return ImageLoadStartEffect::LoadImagePageScopeCandidates;
    }

    return ImageLoadStartEffect::DecodeImage;
}
}

namespace KiriView {
ImagePageScopeLoadPlan imagePageScopeLoadPlan(const ImageLoadRequest &request)
{
    const std::optional<ImagePageScopeLocation> sourceImagePageScope
        = directOpenImagePageScopeLocationForLocalUrl(request.sourceUrl());
    if (sourceImagePageScope.has_value()) {
        return { *sourceImagePageScope, ImagePageScopeLoadEffect::LoadImageCandidates };
    }

    const std::optional<ImagePageScopeLocation> containerImagePageScope
        = containerImagePageScopeForImageLoadRequest(request);
    if (containerImagePageScope.has_value()) {
        return { *containerImagePageScope, ImagePageScopeLoadEffect::ReadImage };
    }

    if (imagePageScopeContainsUrl(request.imagePageScope(), request.sourceUrl())) {
        return { request.imagePageScope(), ImagePageScopeLoadEffect::ReadImage };
    }

    return { ImagePageScopeLocation::none(), ImagePageScopeLoadEffect::ReadImage };
}

ImageLoadPlan imageLoadPlan(
    quint64 id, ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    QUrl sourceUrl = request.sourceUrl();
    ImagePageScopeLoadPlan pageScopePlan = imagePageScopeLoadPlan(request);
    const ImageLoadStartEffect startEffect
        = imageLoadStartEffectForPageScopeEffect(pageScopePlan.effect);
    ImageLoadSession session(id, std::move(request),
        DisplayedImageLocation::fromUrl(
            std::move(sourceUrl), std::move(pageScopePlan.imagePageScope)),
        firstDisplayContext);

    return ImageLoadPlan { std::move(session), startEffect };
}
}
