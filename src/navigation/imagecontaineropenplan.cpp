// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontaineropenplan.h"

#include "location/imagedocumentlocation.h"

namespace KiriView {
bool ImageContainerOpenPlan::shouldLoadCandidates() const { return source.has_value(); }

bool ImageContainerOpenResult::openedImage() const { return imageUrl.has_value(); }

ImageContainerOpenPlan imageContainerOpenPlanForCandidate(
    const ContainerNavigationCandidate &container)
{
    switch (container.type) {
    case ContainerNavigationCandidateType::Directory:
        return { ImageCandidateListSource::forDirectory(container.url),
            ImageContainerOpenError::Generic };
    case ContainerNavigationCandidateType::ComicBookArchive: {
        const std::optional<ArchiveDocumentLocation> archiveDocument
            = archiveDocumentLocationForLocalArchiveUrl(container.url);
        if (archiveDocument.has_value() && archiveDocument->isComicBook()) {
            return { ImageCandidateListSource::forArchiveDocument(*archiveDocument),
                ImageContainerOpenError::Generic };
        }

        return { std::nullopt, ImageContainerOpenError::InvalidComicBookArchive };
    }
    }

    return { std::nullopt, ImageContainerOpenError::Generic };
}

ImageContainerOpenResult imageContainerOpenResultForCandidates(
    const std::vector<ImageNavigationCandidate> &candidates)
{
    if (candidates.empty()) {
        return { std::nullopt, ImageContainerOpenError::EmptyContainer };
    }

    return { candidates.front().url, ImageContainerOpenError::Generic };
}
}
