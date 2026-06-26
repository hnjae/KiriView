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
void appendRemovedCandidate(std::vector<kiriview::ImageDocumentPageCandidate>* candidates,
    const QUrl& currentUrl, const QString& currentName)
{
    candidates->push_back(kiriview::ImageDocumentPageCandidate { currentUrl, currentName });
}

void appendRemovedCandidate(std::vector<kiriview::ContainerNavigationCandidate>* candidates,
    const QUrl& currentUrl, const QString& currentName)
{
    candidates->push_back(kiriview::ContainerNavigationCandidate {
        currentUrl, currentName, kiriview::ContainerNavigationCandidateType::ComicBookArchive });
}

void sortRemovalFallbackCandidates(std::vector<kiriview::ImageDocumentPageCandidate>* candidates)
{
    kiriview::sortImageDocumentPageCandidates(candidates);
}

void sortRemovalFallbackCandidates(std::vector<kiriview::ContainerNavigationCandidate>* candidates)
{
    kiriview::sortContainerNavigationCandidates(candidates);
}

template <typename Candidate>
std::vector<Candidate> removalFallbackCandidates(
    std::vector<Candidate> candidates, const QUrl& currentUrl, const QString& currentName)
{
    appendRemovedCandidate(&candidates, currentUrl, currentName);
    sortRemovalFallbackCandidates(&candidates);

    return candidates;
}

std::optional<kiriview::ImageDocumentPageCandidate> nextFallbackCandidate(
    const std::vector<kiriview::ImageDocumentPageCandidate>& candidates, const QUrl& currentUrl)
{
    return kiriview::adjacentImageDocumentPageCandidate(
        candidates, currentUrl, kiriview::NavigationDirection::Next);
}

std::optional<kiriview::ImageDocumentPageCandidate> previousFallbackCandidate(
    const std::vector<kiriview::ImageDocumentPageCandidate>& candidates, const QUrl& currentUrl)
{
    return kiriview::adjacentImageDocumentPageCandidate(
        candidates, currentUrl, kiriview::NavigationDirection::Previous);
}

std::optional<kiriview::ContainerNavigationCandidate> nextFallbackCandidate(
    const std::vector<kiriview::ContainerNavigationCandidate>& candidates, const QUrl& currentUrl)
{
    return kiriview::adjacentContainerNavigationCandidate(
        candidates, currentUrl, kiriview::NavigationDirection::Next);
}

std::optional<kiriview::ContainerNavigationCandidate> previousFallbackCandidate(
    const std::vector<kiriview::ContainerNavigationCandidate>& candidates, const QUrl& currentUrl)
{
    return kiriview::adjacentContainerNavigationCandidate(
        candidates, currentUrl, kiriview::NavigationDirection::Previous);
}

QUrl removalTargetUrlForDisplayedLocation(const kiriview::DisplayedImageLocation& location)
{
    if (location.imageUrl().isEmpty()) {
        return {};
    }

    if (!location.openedCollectionScope().isEmpty()
        && kiriview::displayedLocationIsInsideOpenedCollectionScope(location)) {
        return location.openedCollectionScopeSourceUrl();
    }

    return location.imageUrl();
}

kiriview::ImageRemovalFallbackPlan removalFallbackPlanForDisplayedLocation(
    const kiriview::DisplayedImageLocation& location)
{
    const std::optional<kiriview::ImageDocumentPageCandidateListContext> imageContext
        = kiriview::imageDocumentPageCandidateListContextForDisplayedImage(location);

    if (kiriview::displayedLocationIsInsideOpenedCollectionScope(location)) {
        if (location.openedCollectionScope().kind()
            == kiriview::OpenedCollectionScopeKind::ComicBookArchive) {
            const QUrl currentContainerUrl = kiriview::containerNavigationUrlForLocation(location);
            return kiriview::ComicBookRemovalFallback { currentContainerUrl,
                kiriview::parentUrlForContainerNavigation(currentContainerUrl),
                currentContainerUrl.fileName() };
        }

        return kiriview::NoImageRemovalFallback {};
    }

    if (imageContext.has_value()) {
        return kiriview::imageRemovalFallbackForImageContext(*imageContext);
    }

    return kiriview::NoImageRemovalFallback {};
}
}

namespace kiriview {
ImageRemovalPlan imageRemovalPlanForDisplayedLocation(const DisplayedImageLocation& location)
{
    const QUrl targetUrl = removalTargetUrlForDisplayedLocation(location);
    if (targetUrl.isEmpty()) {
        return {};
    }

    return ImageRemovalPlan { targetUrl, removalFallbackPlanForDisplayedLocation(location) };
}

ImageRemovalFallback imageRemovalFallbackForImageContext(
    const ImageDocumentPageCandidateListContext& context)
{
    const QUrl currentUrl = context.currentUrl();
    return ImageRemovalFallback { context, currentUrl, currentUrl.fileName() };
}

std::optional<ImageDocumentPageTarget> imageRemovalFallbackTarget(
    std::vector<ImageDocumentPageCandidate> candidates, const ImageRemovalFallback& fallback)
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
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookRemovalFallback& fallback)
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
