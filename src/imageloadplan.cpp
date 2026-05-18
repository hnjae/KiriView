// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadplan.h"

#include "imagecontainer.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
using KiriView::archiveDocumentContainsUrl;
using KiriView::ArchiveDocumentLocation;
using KiriView::archiveDocumentLocationForLocalArchiveUrl;
using KiriView::directOpenDocumentLocationForLocalUrl;
using KiriView::ImageArchiveLoadEffect;
using KiriView::ImageLoadStartEffect;

std::optional<ArchiveDocumentLocation> containerArchiveDocumentForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    if (!request.isContainerNavigation()) {
        return std::nullopt;
    }

    const std::optional<ArchiveDocumentLocation> containerArchive
        = archiveDocumentLocationForLocalArchiveUrl(request.containerNavigationUrl());
    if (containerArchive.has_value()
        && archiveDocumentContainsUrl(*containerArchive, request.sourceUrl())) {
        return containerArchive;
    }

    return std::nullopt;
}

ImageLoadStartEffect imageLoadStartEffectForArchiveEffect(ImageArchiveLoadEffect effect)
{
    switch (effect) {
    case ImageArchiveLoadEffect::ReadImage:
        return ImageLoadStartEffect::DecodeImage;
    case ImageArchiveLoadEffect::LoadImageCandidates:
        return ImageLoadStartEffect::LoadArchiveImageCandidates;
    }

    return ImageLoadStartEffect::DecodeImage;
}
}

namespace KiriView {
ImageArchiveLoadPlan imageArchiveLoadPlan(const ImageLoadRequest &request)
{
    const std::optional<ArchiveDocumentLocation> sourceArchiveDocument
        = directOpenDocumentLocationForLocalUrl(request.sourceUrl());
    if (sourceArchiveDocument.has_value()) {
        return { *sourceArchiveDocument, ImageArchiveLoadEffect::LoadImageCandidates };
    }

    const std::optional<ArchiveDocumentLocation> containerArchiveDocument
        = containerArchiveDocumentForImageLoadRequest(request);
    if (containerArchiveDocument.has_value()) {
        return { *containerArchiveDocument, ImageArchiveLoadEffect::ReadImage };
    }

    if (archiveDocumentContainsUrl(request.archiveDocument(), request.sourceUrl())) {
        return { request.archiveDocument(), ImageArchiveLoadEffect::ReadImage };
    }

    return { ArchiveDocumentLocation::none(), ImageArchiveLoadEffect::ReadImage };
}

ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request)
{
    QUrl sourceUrl = request.sourceUrl();
    ImageArchiveLoadPlan archivePlan = imageArchiveLoadPlan(request);
    const ImageLoadStartEffect startEffect
        = imageLoadStartEffectForArchiveEffect(archivePlan.effect);
    ImageLoadSession session {
        id,
        std::move(request),
        DisplayedImageLocation::fromUrl(
            std::move(sourceUrl), std::move(archivePlan.archiveDocument)),
    };

    return ImageLoadPlan { std::move(session), startEffect };
}
}
