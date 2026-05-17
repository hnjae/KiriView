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
}

namespace KiriView {
ImageArchiveLoadPlan imageArchiveLoadPlan(const ImageLoadRequest &request)
{
    const std::optional<ArchiveDocumentLocation> sourceArchiveDocument
        = directOpenDocumentLocationForLocalUrl(request.sourceUrl());
    if (sourceArchiveDocument.has_value()) {
        return { *sourceArchiveDocument, true };
    }

    const std::optional<ArchiveDocumentLocation> containerArchiveDocument
        = containerArchiveDocumentForImageLoadRequest(request);
    if (containerArchiveDocument.has_value()) {
        return { *containerArchiveDocument, false };
    }

    if (archiveDocumentContainsUrl(request.archiveDocument(), request.sourceUrl())) {
        return { request.archiveDocument(), false };
    }

    return { ArchiveDocumentLocation::none(), false };
}

ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request)
{
    QUrl sourceUrl = request.sourceUrl();
    ImageArchiveLoadPlan archivePlan = imageArchiveLoadPlan(request);
    const bool requiresArchiveListing = archivePlan.requiresArchiveListing;
    ImageLoadSession session {
        id,
        std::move(request),
        DisplayedImageLocation::fromUrl(
            std::move(sourceUrl), std::move(archivePlan.archiveDocument)),
    };

    return ImageLoadPlan { std::move(session), requiresArchiveListing };
}
}
