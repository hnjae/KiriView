// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionfallback.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"
#include "kiriview/src/filedeletionfallback.cxx.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
KiriView::NavigationDirection navigationDirection(KiriView::RustDeletionFallbackDirection direction)
{
    switch (direction) {
    case KiriView::RustDeletionFallbackDirection::Previous:
        return KiriView::NavigationDirection::Previous;
    case KiriView::RustDeletionFallbackDirection::Next:
        return KiriView::NavigationDirection::Next;
    }

    return KiriView::NavigationDirection::Next;
}

std::optional<QUrl> adjacentImageUrlAfterDeletion(
    std::vector<KiriView::ImageNavigationCandidate> candidates, const QUrl &currentUrl,
    const QString &currentName, KiriView::NavigationDirection direction)
{
    candidates.push_back(KiriView::ImageNavigationCandidate { currentUrl, currentName });
    KiriView::sortImageNavigationCandidates(&candidates);
    return KiriView::adjacentImageNavigationUrl(candidates, currentUrl, direction);
}

std::optional<KiriView::ContainerNavigationCandidate> adjacentContainerAfterDeletion(
    std::vector<KiriView::ContainerNavigationCandidate> candidates, const QUrl &currentUrl,
    const QString &currentName, KiriView::NavigationDirection direction)
{
    candidates.push_back(KiriView::ContainerNavigationCandidate {
        currentUrl, currentName, KiriView::ContainerNavigationCandidateType::ComicBookArchive });
    KiriView::sortContainerNavigationCandidates(&candidates);
    return KiriView::adjacentContainerNavigationCandidate(candidates, currentUrl, direction);
}
}

namespace KiriView {
DeletionFallbackPlan deletionFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location)
{
    if (displayedLocationIsInsideArchiveDocument(location)) {
        if (location.archiveDocument().isComicBook()) {
            const QUrl currentContainerUrl = containerNavigationUrlForLocation(location);
            return ComicBookDeletionFallbackPlan { currentContainerUrl,
                currentContainerUrl.fileName() };
        }
        return NoDeletionFallbackPlan {};
    }

    const std::optional<ImageCandidateListContext> imageContext
        = imageCandidateListContextForDisplayedImage(location);
    if (!imageContext.has_value()) {
        return NoDeletionFallbackPlan {};
    }

    const QUrl currentUrl = imageContext->currentUrl();
    return ImageDeletionFallbackPlan { *imageContext, currentUrl, currentUrl.fileName() };
}

std::optional<QUrl> imageDeletionFallbackUrl(
    std::vector<ImageNavigationCandidate> candidates, const ImageDeletionFallbackPlan &plan)
{
    if (plan.currentUrl.isEmpty()) {
        return std::nullopt;
    }

    const RustDeletionFallbackDirections directions = rustDeletionFallbackDirections();
    const std::optional<QUrl> nextUrl = adjacentImageUrlAfterDeletion(
        candidates, plan.currentUrl, plan.currentName, navigationDirection(directions.preferred));
    if (nextUrl.has_value()) {
        return nextUrl;
    }

    return adjacentImageUrlAfterDeletion(std::move(candidates), plan.currentUrl, plan.currentName,
        navigationDirection(directions.fallback));
}

ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookDeletionFallbackPlan &plan)
{
    if (plan.currentContainerUrl.isEmpty()) {
        return {};
    }

    const RustDeletionFallbackDirections directions = rustDeletionFallbackDirections();
    return {
        adjacentContainerAfterDeletion(candidates, plan.currentContainerUrl, plan.currentName,
            navigationDirection(directions.preferred)),
        adjacentContainerAfterDeletion(std::move(candidates), plan.currentContainerUrl,
            plan.currentName, navigationDirection(directions.fallback)),
    };
}
}
