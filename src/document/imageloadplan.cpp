// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadplan.h"

#include "location/imagedocumentlocation.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
using KiriView::ImageLoadStartEffect;
using KiriView::openedCollectionScopeContainsUrl;
using KiriView::OpenedCollectionScopeLoadEffect;
using KiriView::OpenedCollectionScopeLocation;
using KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl;
using KiriView::openedCollectionScopeLocationForLocalArchiveUrl;

std::optional<OpenedCollectionScopeLocation> containerOpenedCollectionScopeForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    if (!request.isContainerNavigation()) {
        return std::nullopt;
    }

    const std::optional<OpenedCollectionScopeLocation> containerOpenedCollectionScope
        = openedCollectionScopeLocationForLocalArchiveUrl(request.containerNavigationUrl());
    if (containerOpenedCollectionScope.has_value()
        && openedCollectionScopeContainsUrl(*containerOpenedCollectionScope, request.sourceUrl())) {
        return containerOpenedCollectionScope;
    }

    return std::nullopt;
}

ImageLoadStartEffect imageLoadStartEffectForPageScopeEffect(OpenedCollectionScopeLoadEffect effect)
{
    switch (effect) {
    case OpenedCollectionScopeLoadEffect::ReadImage:
        return ImageLoadStartEffect::DecodeImage;
    case OpenedCollectionScopeLoadEffect::LoadImageDocumentPageCandidates:
        return ImageLoadStartEffect::LoadOpenedCollectionScopeCandidates;
    }

    return ImageLoadStartEffect::DecodeImage;
}
}

namespace KiriView {
OpenedCollectionScopeLoadPlan openedCollectionScopeLoadPlan(const ImageLoadRequest &request)
{
    const std::optional<OpenedCollectionScopeLocation> sourceOpenedCollectionScope
        = openedCollectionScopeLocationForDirectlyOpenedLocalUrl(request.sourceUrl());
    if (sourceOpenedCollectionScope.has_value()) {
        return { *sourceOpenedCollectionScope,
            OpenedCollectionScopeLoadEffect::LoadImageDocumentPageCandidates };
    }

    const std::optional<OpenedCollectionScopeLocation> containerOpenedCollectionScope
        = containerOpenedCollectionScopeForImageLoadRequest(request);
    if (containerOpenedCollectionScope.has_value()) {
        return { *containerOpenedCollectionScope, OpenedCollectionScopeLoadEffect::ReadImage };
    }

    if (openedCollectionScopeContainsUrl(request.openedCollectionScope(), request.sourceUrl())) {
        return { request.openedCollectionScope(), OpenedCollectionScopeLoadEffect::ReadImage };
    }

    return { OpenedCollectionScopeLocation::none(), OpenedCollectionScopeLoadEffect::ReadImage };
}

ImageLoadPlan imageLoadPlan(
    quint64 id, ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    QUrl sourceUrl = request.sourceUrl();
    OpenedCollectionScopeLoadPlan pageScopePlan = openedCollectionScopeLoadPlan(request);
    const ImageLoadStartEffect startEffect
        = imageLoadStartEffectForPageScopeEffect(pageScopePlan.effect);
    ImageLoadSession session(id, std::move(request),
        DisplayedImageLocation::fromUrl(
            std::move(sourceUrl), std::move(pageScopePlan.openedCollectionScope)),
        firstDisplayContext);

    return ImageLoadPlan { std::move(session), startEffect };
}
}
