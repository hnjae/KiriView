// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionfallback.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
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

    const std::optional<QUrl> nextUrl = adjacentImageUrlAfterDeletion(
        candidates, plan.currentUrl, plan.currentName, NavigationDirection::Next);
    if (nextUrl.has_value()) {
        return nextUrl;
    }

    return adjacentImageUrlAfterDeletion(
        std::move(candidates), plan.currentUrl, plan.currentName, NavigationDirection::Previous);
}

ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookDeletionFallbackPlan &plan)
{
    if (plan.currentContainerUrl.isEmpty()) {
        return {};
    }

    return {
        adjacentContainerAfterDeletion(
            candidates, plan.currentContainerUrl, plan.currentName, NavigationDirection::Next),
        adjacentContainerAfterDeletion(std::move(candidates), plan.currentContainerUrl,
            plan.currentName, NavigationDirection::Previous),
    };
}
}
