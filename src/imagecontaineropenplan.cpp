// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontaineropenplan.h"

#include "imagecontainer.h"

namespace KiriView {
bool ImageContainerOpenPlan::shouldLoadCandidates() const { return source.has_value(); }

bool ImageContainerOpenResult::openedImage() const { return imageUrl.has_value(); }

ImageContainerOpenPlan imageContainerOpenPlanForCandidate(
    const ContainerNavigationCandidate &container)
{
    switch (container.type) {
    case ContainerNavigationCandidateType::Directory:
        return { ImageCandidateListSource::forDirectory(container.url),
            ImageCandidateRepositoryError::Generic };
    case ContainerNavigationCandidateType::ComicBookArchive: {
        const std::optional<ArchiveDocumentLocation> archiveDocument
            = archiveDocumentLocationForLocalArchiveUrl(container.url);
        if (archiveDocument.has_value() && archiveDocument->isComicBook()) {
            return { ImageCandidateListSource::forArchiveDocument(*archiveDocument),
                ImageCandidateRepositoryError::Generic };
        }

        return { std::nullopt, ImageCandidateRepositoryError::InvalidComicBookArchive };
    }
    }

    return { std::nullopt, ImageCandidateRepositoryError::Generic };
}

ImageContainerOpenResult imageContainerOpenResultForCandidates(
    const std::vector<ImageNavigationCandidate> &candidates)
{
    if (candidates.empty()) {
        return { std::nullopt, ImageCandidateRepositoryError::EmptyContainer };
    }

    return { candidates.front().url, ImageCandidateRepositoryError::Generic };
}
}
