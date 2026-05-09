// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadplan.h"

#include "imagecontainer.h"
#include "kiriview/src/imageloadplan.cxx.h"

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

ArchiveDocumentLocation archiveDocumentForImageLoadTarget(
    KiriView::RustImageLoadArchiveDocumentTarget target,
    const std::optional<ArchiveDocumentLocation> &sourceArchiveDocument,
    const std::optional<ArchiveDocumentLocation> &containerArchiveDocument,
    const ArchiveDocumentLocation &displayedArchiveDocument)
{
    switch (target) {
    case KiriView::RustImageLoadArchiveDocumentTarget::SourceArchive:
        return sourceArchiveDocument.value_or(ArchiveDocumentLocation::none());
    case KiriView::RustImageLoadArchiveDocumentTarget::ContainerArchive:
        return containerArchiveDocument.value_or(ArchiveDocumentLocation::none());
    case KiriView::RustImageLoadArchiveDocumentTarget::DisplayedArchive:
        return displayedArchiveDocument;
    case KiriView::RustImageLoadArchiveDocumentTarget::None:
        break;
    }

    return ArchiveDocumentLocation::none();
}

ArchiveDocumentLoadPlan archiveDocumentLoadPlanForImageLoadRequest(
    const KiriView::ImageLoadRequest &request)
{
    const std::optional<ArchiveDocumentLocation> sourceArchiveDocument
        = archiveDocumentLocationForLocalArchiveUrl(request.sourceUrl());
    const std::optional<ArchiveDocumentLocation> containerArchiveDocument
        = containerArchiveDocumentForImageLoadRequest(request);
    const bool displayedArchiveContainsSource
        = archiveDocumentContainsUrl(request.archiveDocument(), request.sourceUrl());
    const KiriView::RustImageLoadPlan plan
        = KiriView::rustImageLoadPlan(sourceArchiveDocument.has_value(),
            containerArchiveDocument.has_value(), displayedArchiveContainsSource);

    return ArchiveDocumentLoadPlan {
        archiveDocumentForImageLoadTarget(plan.archive_document, sourceArchiveDocument,
            containerArchiveDocument, request.archiveDocument()),
        plan.requires_archive_listing,
    };
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
