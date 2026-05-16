// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageremovalfallback.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"
#include "kiriview/src/imagedeletionpolicy.cxx.h"

#include <cstdint>
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

std::uint8_t rustFlag(bool value) { return value ? 1 : 0; }

KiriView::RustArchiveDocumentKind rustArchiveDocumentKind(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return KiriView::RustArchiveDocumentKind::Empty;
    }

    switch (archiveDocument.kind()) {
    case KiriView::ArchiveDocumentKind::ComicBook:
        return KiriView::RustArchiveDocumentKind::ComicBook;
    case KiriView::ArchiveDocumentKind::General:
        return KiriView::RustArchiveDocumentKind::General;
    case KiriView::ArchiveDocumentKind::Directory:
        return KiriView::RustArchiveDocumentKind::Directory;
    }

    return KiriView::RustArchiveDocumentKind::Empty;
}

KiriView::RustImageRemovalFallbackPlanInput rustImageRemovalFallbackPlanInput(
    const KiriView::DisplayedImageLocation &location,
    const std::optional<KiriView::ImageCandidateListContext> &imageContext)
{
    return KiriView::RustImageRemovalFallbackPlanInput {
        rustArchiveDocumentKind(location.archiveDocument()),
        KiriView::displayedLocationIsInsideArchiveDocument(location),
        imageContext.has_value(),
    };
}

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

std::optional<std::size_t> fallbackIndexValue(KiriView::RustImageRemovalFallbackIndex index)
{
    if (!index.found) {
        return std::nullopt;
    }

    return index.index;
}

template <typename Candidate>
KiriView::RustImageRemovalFallbackCandidateIndices removalFallbackCandidateIndices(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::rustImageRemovalFallbackCandidateIndices(
        currentCandidateMatches(candidates, currentUrl));
}

template <typename Candidate>
std::optional<Candidate> candidateAt(
    const std::vector<Candidate> &candidates, KiriView::RustImageRemovalFallbackIndex index)
{
    const std::optional<std::size_t> candidateIndex = fallbackIndexValue(index);
    if (!candidateIndex.has_value() || *candidateIndex >= candidates.size()) {
        return std::nullopt;
    }

    return candidates.at(*candidateIndex);
}
}

namespace KiriView {
ImageRemovalFallbackPlan imageRemovalFallbackPlanForDisplayedLocation(
    const DisplayedImageLocation &location)
{
    const std::optional<ImageCandidateListContext> imageContext
        = imageCandidateListContextForDisplayedImage(location);

    switch (rustImageRemovalFallbackPlanKind(
        rustImageRemovalFallbackPlanInput(location, imageContext))) {
    case RustImageRemovalFallbackPlanKind::ComicBook: {
        const QUrl currentContainerUrl = containerNavigationUrlForLocation(location);
        return ComicBookRemovalFallback { currentContainerUrl,
            parentUrlForContainerNavigation(currentContainerUrl), currentContainerUrl.fileName() };
    }
    case RustImageRemovalFallbackPlanKind::Image:
        if (imageContext.has_value()) {
            return imageRemovalFallbackForImageContext(*imageContext);
        }
        return NoImageRemovalFallback {};
    case RustImageRemovalFallbackPlanKind::NoFallback:
        return NoImageRemovalFallback {};
    }

    return NoImageRemovalFallback {};
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
    const RustImageRemovalFallbackCandidateIndices selected
        = removalFallbackCandidateIndices(fallbackCandidates, fallback.currentUrl);

    const std::optional<ImageNavigationCandidate> preferred
        = candidateAt(fallbackCandidates, selected.preferred);
    if (preferred.has_value()) {
        return preferred->url;
    }

    const std::optional<ImageNavigationCandidate> previous
        = candidateAt(fallbackCandidates, selected.fallback);
    return previous.has_value() ? std::optional<QUrl>(previous->url) : std::nullopt;
}

ComicBookRemovalFallbackCandidates comicBookRemovalFallbackCandidates(
    std::vector<ContainerNavigationCandidate> candidates, const ComicBookRemovalFallback &fallback)
{
    if (fallback.currentContainerUrl.isEmpty()) {
        return {};
    }

    const std::vector<ContainerNavigationCandidate> fallbackCandidates = removalFallbackCandidates(
        std::move(candidates), fallback.currentContainerUrl, fallback.currentName);
    const RustImageRemovalFallbackCandidateIndices selected
        = removalFallbackCandidateIndices(fallbackCandidates, fallback.currentContainerUrl);
    return { candidateAt(fallbackCandidates, selected.preferred),
        candidateAt(fallbackCandidates, selected.fallback) };
}
}
