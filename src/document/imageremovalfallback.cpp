// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageremovalfallback.h"

#include "location/imagedocumentlocation.h"
#include "location/imageurl.h"
#include "navigation/imagedocumentpagenavigationpolicy.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
void appendRemovedCandidate(std::vector<KiriView::ImageDocumentPageCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ImageDocumentPageCandidate { currentUrl, currentName });
}

void appendRemovedCandidate(std::vector<KiriView::ContainerNavigationCandidate> *candidates,
    const QUrl &currentUrl, const QString &currentName)
{
    candidates->push_back(KiriView::ContainerNavigationCandidate {
        currentUrl, currentName, KiriView::ContainerNavigationCandidateType::ComicBookArchive });
}

void sortRemovalFallbackCandidates(std::vector<KiriView::ImageDocumentPageCandidate> *candidates)
{
    KiriView::sortImageDocumentPageCandidates(candidates);
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

std::optional<KiriView::ImageDocumentPageCandidate> nextFallbackCandidate(
    const std::vector<KiriView::ImageDocumentPageCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentImageDocumentPageCandidate(
        candidates, currentUrl, KiriView::NavigationDirection::Next);
}

std::optional<KiriView::ImageDocumentPageCandidate> previousFallbackCandidate(
    const std::vector<KiriView::ImageDocumentPageCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::adjacentImageDocumentPageCandidate(
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

    if (!location.openedCollectionScope().isEmpty()
        && KiriView::displayedLocationIsInsideOpenedCollectionScope(location)) {
        return location.openedCollectionScopeSourceUrl();
    }

    return location.imageUrl();
}

KiriView::ImageRemovalFallbackPlan removalFallbackPlanForDisplayedLocation(
    const KiriView::DisplayedImageLocation &location)
{
    const std::optional<KiriView::ImageDocumentPageCandidateListContext> imageContext
        = KiriView::imageDocumentPageCandidateListContextForDisplayedImage(location);

    if (KiriView::displayedLocationIsInsideOpenedCollectionScope(location)) {
        if (location.openedCollectionScope().kind()
            == KiriView::OpenedCollectionScopeKind::ComicBookArchive) {
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

ImageRemovalFallback imageRemovalFallbackForImageContext(
    const ImageDocumentPageCandidateListContext &context)
{
    const QUrl currentUrl = context.currentUrl();
    return ImageRemovalFallback { context, currentUrl, currentUrl.fileName() };
}

std::optional<ImageDocumentPageTarget> imageRemovalFallbackTarget(
    std::vector<ImageDocumentPageCandidate> candidates, const ImageRemovalFallback &fallback)
{
    if (fallback.currentUrl.isEmpty()) {
        return std::nullopt;
    }

    const std::vector<ImageDocumentPageCandidate> fallbackCandidates = removalFallbackCandidates(
        std::move(candidates), fallback.currentUrl, fallback.currentName);

    const std::optional<ImageDocumentPageCandidate> preferred
        = nextFallbackCandidate(fallbackCandidates, fallback.currentUrl);
    if (preferred.has_value()) {
        return ImageDocumentPageTarget { preferred->url, preferred->kind };
    }

    const std::optional<ImageDocumentPageCandidate> fallbackCandidate
        = previousFallbackCandidate(fallbackCandidates, fallback.currentUrl);
    if (!fallbackCandidate.has_value()) {
        return std::nullopt;
    }

    return ImageDocumentPageTarget { fallbackCandidate->url, fallbackCandidate->kind };
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
