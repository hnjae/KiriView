// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionfallback.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
void appendDeletedCandidate(std::vector<KiriView::ImageNavigationCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ImageNavigationCandidate { currentUrl, currentName });
}

void appendDeletedCandidate(std::vector<KiriView::ContainerNavigationCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ContainerNavigationCandidate {
        currentUrl, currentName, KiriView::ContainerNavigationCandidateType::ComicBookArchive });
}

void sortDeletionFallbackCandidates(std::vector<KiriView::ImageNavigationCandidate> *candidates)
{
    KiriView::sortImageNavigationCandidates(candidates);
}

void sortDeletionFallbackCandidates(std::vector<KiriView::ContainerNavigationCandidate> *candidates)
{
    KiriView::sortContainerNavigationCandidates(candidates);
}

template <typename Candidate>
std::vector<Candidate> deletionFallbackCandidates(
    std::vector<Candidate> candidates, const QUrl &currentUrl, const QString &currentName)
{
    appendDeletedCandidate(&candidates, currentUrl, currentName);
    sortDeletionFallbackCandidates(&candidates);

    return candidates;
}

std::optional<QUrl> preferredImageFallbackUrl(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const std::optional<QUrl> preferred = KiriView::adjacentImageNavigationUrl(
        candidates, currentUrl, KiriView::NavigationDirection::Next);
    if (preferred.has_value()) {
        return preferred;
    }

    return KiriView::adjacentImageNavigationUrl(
        candidates, currentUrl, KiriView::NavigationDirection::Previous);
}
}

namespace KiriView {
DeletionFallbackPlan deletionFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location)
{
    const bool insideArchiveDocument = displayedLocationIsInsideArchiveDocument(location);
    if (insideArchiveDocument) {
        if (!location.archiveDocument().isComicBook()) {
            return NoDeletionFallbackPlan {};
        }

        const QUrl currentContainerUrl = containerNavigationUrlForLocation(location);
        return ComicBookDeletionFallbackPlan { currentContainerUrl,
            currentContainerUrl.fileName() };
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

    return preferredImageFallbackUrl(
        deletionFallbackCandidates(std::move(candidates), plan.currentUrl, plan.currentName),
        plan.currentUrl);
}

ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookDeletionFallbackPlan &plan)
{
    if (plan.currentContainerUrl.isEmpty()) {
        return {};
    }

    const std::vector<ContainerNavigationCandidate> fallbackCandidates = deletionFallbackCandidates(
        std::move(candidates), plan.currentContainerUrl, plan.currentName);
    return {
        adjacentContainerNavigationCandidate(
            fallbackCandidates, plan.currentContainerUrl, NavigationDirection::Next),
        adjacentContainerNavigationCandidate(
            fallbackCandidates, plan.currentContainerUrl, NavigationDirection::Previous),
    };
}
}
