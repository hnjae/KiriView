// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageremovalfallback.h"

#include "location/imagedocumentlocation.h"
#include "location/imageurl.h"
#include "navigation/imagenavigationmodel.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
void appendRemovedCandidate(std::vector<KiriView::ImageNavigationCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ImageNavigationCandidate { currentUrl, currentName });
}

void appendRemovedCandidate(std::vector<KiriView::ContainerNavigationCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ContainerNavigationCandidate {
        currentUrl, currentName, KiriView::ContainerNavigationCandidateType::ComicBookArchive });
}

void sortRemovalFallbackCandidates(std::vector<KiriView::ImageNavigationCandidate> *candidates)
{
    KiriView::sortImageNavigationCandidates(candidates);
}

void sortRemovalFallbackCandidates(std::vector<KiriView::ContainerNavigationCandidate> *candidates)
{
    KiriView::sortContainerNavigationCandidates(candidates);
}

template <typename Candidate>
std::vector<Candidate> removalFallbackCandidates(
    std::vector<Candidate> candidates, const QUrl &currentUrl, const QString &currentName)
{
    appendRemovedCandidate(&candidates, currentUrl, currentName);
    sortRemovalFallbackCandidates(&candidates);

    return candidates;
}

std::optional<QUrl> nextFallbackUrl(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentImageNavigationUrl(
        candidates, currentUrl, KiriView::NavigationDirection::Next);
}

std::optional<QUrl> previousFallbackUrl(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentImageNavigationUrl(
        candidates, currentUrl, KiriView::NavigationDirection::Previous);
}

std::optional<KiriView::ContainerNavigationCandidate> nextFallbackCandidate(
    const std::vector<KiriView::ContainerNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentContainerNavigationCandidate(
        candidates, currentUrl, KiriView::NavigationDirection::Next);
}

std::optional<KiriView::ContainerNavigationCandidate> previousFallbackCandidate(
    const std::vector<KiriView::ContainerNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentContainerNavigationCandidate(
        candidates, currentUrl, KiriView::NavigationDirection::Previous);
}

QUrl removalTargetUrlForDisplayedLocation(const KiriView::DisplayedImageLocation &location)
{
    if (location.imageUrl().isEmpty()) {
        return {};
    }

    if (!location.imagePageScope().isEmpty()
        && KiriView::displayedLocationIsInsideImagePageScope(location)) {
        return location.imagePageScopeSourceUrl();
    }

    return location.imageUrl();
}

KiriView::ImageRemovalFallbackPlan removalFallbackPlanForDisplayedLocation(
    const KiriView::DisplayedImageLocation &location)
{
    const std::optional<KiriView::ImageCandidateListContext> imageContext
        = KiriView::imageCandidateListContextForDisplayedImage(location);

    if (KiriView::displayedLocationIsInsideImagePageScope(location)) {
        if (location.imagePageScope().kind() == KiriView::ImagePageScopeKind::ComicBookArchive) {
            const QUrl currentContainerUrl = KiriView::containerNavigationUrlForLocation(location);
            return KiriView::ComicBookRemovalFallback { currentContainerUrl,
                KiriView::parentUrlForContainerNavigation(currentContainerUrl),
                currentContainerUrl.fileName() };
        }

        return KiriView::NoImageRemovalFallback {};
    }

    if (imageContext.has_value()) {
        return KiriView::imageRemovalFallbackForImageContext(*imageContext);
    }

    return KiriView::NoImageRemovalFallback {};
}
}

namespace KiriView {
ImageRemovalPlan imageRemovalPlanForDisplayedLocation(const DisplayedImageLocation &location)
{
    const QUrl targetUrl = removalTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return {};
    }

    return ImageRemovalPlan { targetUrl, removalFallbackPlanForDisplayedLocation(location) };
}

ImageRemovalFallback imageRemovalFallbackForImageContext(const ImageCandidateListContext &context)
{
    const QUrl currentUrl = context.currentUrl();
    return ImageRemovalFallback { context, currentUrl, currentUrl.fileName() };
}

std::optional<QUrl> imageRemovalFallbackUrl(
    std::vector<ImageNavigationCandidate> candidates, const ImageRemovalFallback &fallback)
{
    if (fallback.currentUrl.isEmpty()) {
        return std::nullopt;
    }

    const std::vector<ImageNavigationCandidate> fallbackCandidates = removalFallbackCandidates(
        std::move(candidates), fallback.currentUrl, fallback.currentName);

    const std::optional<QUrl> preferred = nextFallbackUrl(fallbackCandidates, fallback.currentUrl);
    if (preferred.has_value()) {
        return preferred;
    }

    return previousFallbackUrl(fallbackCandidates, fallback.currentUrl);
}

ComicBookRemovalFallbackCandidates comicBookRemovalFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookRemovalFallback &fallback)
{
    if (fallback.currentContainerUrl.isEmpty()) {
        return {};
    }

    const std::vector<ContainerNavigationCandidate> fallbackCandidates = removalFallbackCandidates(
        std::move(candidates), fallback.currentContainerUrl, fallback.currentName);
    return { nextFallbackCandidate(fallbackCandidates, fallback.currentContainerUrl),
        previousFallbackCandidate(fallbackCandidates, fallback.currentContainerUrl) };
}
}
