// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontaineropenplan.h"

#include "location/imagedocumentlocation.h"

namespace kiriview {
bool ImageContainerOpenPlan::shouldLoadCandidates() const { return source.has_value(); }

bool ImageContainerOpenResult::openedImage() const { return target.has_value(); }

ImageContainerOpenPlan imageContainerOpenPlanForCandidate(
    const ContainerNavigationCandidate &container)
{
    switch (container.type) {
    case ContainerNavigationCandidateType::Directory:
        return { ImageDocumentPageCandidateListSource::forDirectory(container.url),
            ImageContainerOpenError::Generic };
    case ContainerNavigationCandidateType::ComicBookArchive: {
        const std::optional<OpenedCollectionScopeLocation> openedCollectionScope
            = openedCollectionScopeLocationForLocalArchiveUrl(container.url);
        if (openedCollectionScope.has_value() && openedCollectionScope->isComicBook()) {
            return { ImageDocumentPageCandidateListSource::forOpenedCollectionScope(
                         *openedCollectionScope),
                ImageContainerOpenError::Generic };
        }

        return { std::nullopt, ImageContainerOpenError::InvalidComicBookArchive };
    }
    }

    return { std::nullopt, ImageContainerOpenError::Generic };
}

ImageContainerOpenResult imageContainerOpenResultForCandidates(
    const std::vector<ImageDocumentPageCandidate> &candidates)
{
    if (candidates.empty()) {
        return { std::nullopt, ImageContainerOpenError::EmptyContainer };
    }

    const ImageDocumentPageCandidate &candidate = candidates.front();
    return { ImageDocumentPageTarget { candidate.url, candidate.kind },
        ImageContainerOpenError::Generic };
}
}
