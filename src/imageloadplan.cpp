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
}

namespace KiriView {
ImageLoadPlan imageLoadPlan(quint64 id, ImageLoadRequest request)
{
    const QUrl sourceUrl = request.sourceUrl();
    ImageLoadSession session {
        id,
        std::move(request),
        DisplayedImageLocation::fromUrl(sourceUrl),
    };

    const std::optional<ArchiveDocumentLocation> selectedArchiveDocument
        = archiveDocumentLocationForLocalArchiveUrl(sourceUrl);
    if (selectedArchiveDocument.has_value()) {
        session.location.setArchiveDocument(*selectedArchiveDocument);
        return ImageLoadPlan { std::move(session), true };
    }

    session.location.setArchiveDocument(archiveDocumentForImageLoadRequest(session.request));
    return ImageLoadPlan { std::move(session), false };
}
}
