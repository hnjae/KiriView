// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletionfallback.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"
#include "kiriview/src/filedeletionfallback.cxx.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace {
std::uint8_t rustFlag(bool value) { return value ? 1 : 0; }

template <typename Candidate>
rust::Vec<std::uint8_t> currentCandidateMatches(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    rust::Vec<std::uint8_t> matches;
    matches.reserve(candidates.size());
    for (const Candidate &candidate : candidates) {
        matches.push_back(rustFlag(KiriView::sameNormalizedUrl(candidate.url, currentUrl)));
    }

    return matches;
}

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

template <typename Candidate> struct DeletionFallbackSelection {
    std::vector<Candidate> candidates;
    KiriView::RustDeletionFallbackCandidateIndices indices;
};

template <typename Candidate>
DeletionFallbackSelection<Candidate> deletionFallbackSelection(
    std::vector<Candidate> candidates, const QUrl &currentUrl, const QString &currentName)
{
    appendDeletedCandidate(&candidates, currentUrl, currentName);
    sortDeletionFallbackCandidates(&candidates);

    const KiriView::RustDeletionFallbackCandidateIndices indices
        = KiriView::rustDeletionFallbackCandidateIndicesForMatches(
            currentCandidateMatches(candidates, currentUrl));
    return { std::move(candidates), indices };
}

template <typename Candidate>
std::optional<std::size_t> candidateIndex(
    const std::vector<Candidate> &candidates, KiriView::RustDeletionFallbackIndex index)
{
    if (!index.found || index.index >= candidates.size()) {
        return std::nullopt;
    }

    return index.index;
}

std::optional<QUrl> preferredImageFallbackUrl(
    const DeletionFallbackSelection<KiriView::ImageNavigationCandidate> &selection)
{
    const std::optional<std::size_t> preferred
        = candidateIndex(selection.candidates, selection.indices.preferred);
    if (preferred.has_value()) {
        return selection.candidates.at(*preferred).url;
    }

    const std::optional<std::size_t> fallback
        = candidateIndex(selection.candidates, selection.indices.fallback);
    if (fallback.has_value()) {
        return selection.candidates.at(*fallback).url;
    }

    return std::nullopt;
}

std::optional<KiriView::ContainerNavigationCandidate> containerFallbackCandidate(
    const DeletionFallbackSelection<KiriView::ContainerNavigationCandidate> &selection,
    KiriView::RustDeletionFallbackIndex index)
{
    const std::optional<std::size_t> candidate = candidateIndex(selection.candidates, index);
    if (!candidate.has_value()) {
        return std::nullopt;
    }

    return selection.candidates.at(*candidate);
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
        deletionFallbackSelection(std::move(candidates), plan.currentUrl, plan.currentName));
}

ComicBookDeletionFallbackCandidates comicBookDeletionFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookDeletionFallbackPlan &plan)
{
    if (plan.currentContainerUrl.isEmpty()) {
        return {};
    }

    const DeletionFallbackSelection<ContainerNavigationCandidate> selection
        = deletionFallbackSelection(
            std::move(candidates), plan.currentContainerUrl, plan.currentName);
    return {
        containerFallbackCandidate(selection, selection.indices.preferred),
        containerFallbackCandidate(selection, selection.indices.fallback),
    };
}
}
