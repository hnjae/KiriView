// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadplan.h"

#include "location/imagedocumentlocation.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
using kiriview::ImageLoadStartEffect;
using kiriview::openedCollectionScopeContainsUrl;
using kiriview::OpenedCollectionScopeLoadEffect;
using kiriview::OpenedCollectionScopeLocation;
using kiriview::openedCollectionScopeLocationForLocalArchiveUrl;

std::optional<OpenedCollectionScopeLocation> containerOpenedCollectionScopeForImageLoadRequest(
    const kiriview::ImageLoadRequest& request)
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

namespace kiriview {
OpenedCollectionScopeLoadPlan openedCollectionScopeLoadPlan(
    const ImageLoadRequest& request, const ImageLoadResolvedSourceFacts& resolvedSourceFacts)
{
    const std::optional<OpenedCollectionScopeLocation> sourceOpenedCollectionScope
        = resolvedSourceFacts.directlyOpenedCollectionScope;
    if (sourceOpenedCollectionScope.has_value()) {
        return { *sourceOpenedCollectionScope,
            OpenedCollectionScopeLoadEffect::LoadImageDocumentPageCandidates };
    }

    const std::optional<OpenedCollectionScopeLocation> archiveOpenedCollectionScope
        = openedCollectionScopeLocationForLocalArchiveUrl(request.sourceUrl());
    if (archiveOpenedCollectionScope.has_value()) {
        return { *archiveOpenedCollectionScope,
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

ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request,
    ImageFirstDisplayDecodeContext firstDisplayContext,
    ImageLoadResolvedSourceFacts resolvedSourceFacts)
{
    QUrl sourceUrl = request.sourceUrl();
    OpenedCollectionScopeLoadPlan pageScopePlan
        = openedCollectionScopeLoadPlan(request, resolvedSourceFacts);
    const ImageLoadStartEffect startEffect
        = imageLoadStartEffectForPageScopeEffect(pageScopePlan.effect);
    ImageLoadSession session(id, std::move(request),
        DisplayedImageLocation::fromUrl(
            std::move(sourceUrl), std::move(pageScopePlan.openedCollectionScope)),
        firstDisplayContext);

    return ImageLoadPlan { std::move(session), startEffect };
}
}
