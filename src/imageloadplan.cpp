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

struct ArchiveDocumentLoadPlan {
    ArchiveDocumentLocation archiveDocument;
    bool requiresArchiveListing = false;
};

ArchiveDocumentLocation archiveDocumentForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    if (request.isContainerNavigation()) {
        const std::optional<ArchiveDocumentLocation> containerArchive
            = archiveDocumentLocationForLocalArchiveUrl(request.containerNavigationUrl());
        if (containerArchive.has_value()
            && archiveDocumentContainsUrl(*containerArchive, request.sourceUrl())) {
            return *containerArchive;
        }
    }

    if (archiveDocumentContainsUrl(request.archiveDocument(), request.sourceUrl())) {
        return request.archiveDocument();
    }

    return ArchiveDocumentLocation::none();
}

ArchiveDocumentLoadPlan archiveDocumentLoadPlanForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    const std::optional<ArchiveDocumentLocation> selectedArchiveDocument
        = archiveDocumentLocationForLocalArchiveUrl(request.sourceUrl());
    if (selectedArchiveDocument.has_value()) {
        return ArchiveDocumentLoadPlan { *selectedArchiveDocument, true };
    }

    return ArchiveDocumentLoadPlan { archiveDocumentForImageLoadRequest(request), false };
}
}

namespace KiriView {
ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request)
{
    QUrl sourceUrl = request.sourceUrl();
    ArchiveDocumentLoadPlan archivePlan = archiveDocumentLoadPlanForImageLoadRequest(request);
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
