// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imagecallback.h"
#include "imagenavigationmodel.h"
#include "imageremovalfallback.h"
#include "imageurl.h"

#include <QString>
#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace {
bool displayContextHasNavigationSource(
    const KiriView::ImageNavigationService::DisplayContext &context)
{
    return context.hasDisplayedImage && !context.location.imageUrl().isEmpty();
}

std::optional<KiriView::ImageCandidateListContext> imageCandidateContextForDisplayContext(
    const KiriView::ImageNavigationService::DisplayContext &context)
{
    if (!displayContextHasNavigationSource(context)) {
        return std::nullopt;
    }

    return KiriView::imageCandidateListContextForDisplayedImage(context.location);
}

bool samePageNavigationState(
    const KiriView::PageNavigationState &left, const KiriView::PageNavigationState &right)
{
    return left.urls == right.urls && left.currentIndex == right.currentIndex;
}

std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const KiriView::ImageNavigationCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

bool imageNavigationCandidatesContainUrl(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &url)
{
    return std::any_of(candidates.cbegin(), candidates.cend(),
        [&url](const KiriView::ImageNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, url);
        });
}

bool sameArchiveDocumentLocation(
    const KiriView::ArchiveDocumentLocation &left, const KiriView::ArchiveDocumentLocation &right)
{
    return KiriView::sameNormalizedUrl(left.fileUrl(), right.fileUrl())
        && KiriView::sameNormalizedUrl(left.rootUrl(), right.rootUrl())
        && left.kind() == right.kind();
}

bool sameImageCandidateListSourcePayload(const KiriView::ImageCandidateListSource::Directory &left,
    const KiriView::ImageCandidateListSource::Directory &right)
{
    return KiriView::sameNormalizedUrl(left.directoryUrl, right.directoryUrl);
}

bool sameImageCandidateListSourcePayload(
    const KiriView::ImageCandidateListSource::ArchiveDocument &left,
    const KiriView::ImageCandidateListSource::ArchiveDocument &right)
{
    return sameArchiveDocumentLocation(left.archiveDocument, right.archiveDocument);
}

template <typename Left, typename Right>
bool sameImageCandidateListSourcePayload(const Left &, const Right &)
{
    return false;
}

bool sameImageCandidateListSource(
    const KiriView::ImageCandidateListSource &left, const KiriView::ImageCandidateListSource &right)
{
    return left.visit([&right](const auto &leftSource) {
        return right.visit([&leftSource](const auto &rightSource) {
            return sameImageCandidateListSourcePayload(leftSource, rightSource);
        });
    });
}
}

namespace KiriView {
ImageNavigationService::ImageNavigationService(
    QObject *parent, ImageNavigationCandidateProvider candidateProvider, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
{
}

int ImageNavigationService::currentPageNumber() const
{
    return m_pageNavigation.currentIndex < 0 ? 0 : m_pageNavigation.currentIndex + 1;
}

int ImageNavigationService::imageCount() const
{
    return static_cast<int>(m_pageNavigation.urls.size());
}

std::optional<QUrl> ImageNavigationService::urlAtPage(int pageNumber) const
{
    if (pageNumber < 1) {
        return std::nullopt;
    }

    const std::size_t pageIndex = static_cast<std::size_t>(pageNumber - 1);
    if (pageIndex >= m_pageNavigation.urls.size()) {
        return std::nullopt;
    }

    return m_pageNavigation.urls.at(pageIndex);
}

std::optional<QUrl> ImageNavigationService::selectPage(int pageNumber)
{
    return selectPageTarget(pageNavigationTargetIndex(m_pageNavigation, pageNumber));
}

std::optional<QUrl> ImageNavigationService::selectPageTarget(std::optional<std::size_t> targetIndex)
{
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    PageNavigationState state = m_pageNavigation;
    state.currentIndex = static_cast<int>(*targetIndex);
    const QUrl targetUrl = state.urls.at(*targetIndex);
    setPageNavigationState(std::move(state));
    return targetUrl;
}

bool ImageNavigationService::hasKnownPageNavigationSelection() const
{
    if (m_pageNavigation.currentIndex < 0) {
        return false;
    }

    return static_cast<std::size_t>(m_pageNavigation.currentIndex) < m_pageNavigation.urls.size();
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    cancelContainerNavigation();
    cancelNavigation();

    if (hasKnownPageNavigationSelection()) {
        const std::optional<QUrl> targetUrl
            = selectPageTarget(pageNavigationAdjacentTargetIndex(m_pageNavigation, direction));
        if (targetUrl.has_value()) {
            invokeIfSet(m_callbacks.openUrl, *targetUrl);
        }
        return;
    }

    if (!displayContextHasNavigationSource(context)) {
        return;
    }

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateContextForDisplayContext(context);
    if (!candidateContext.has_value()) {
        return;
    }

    m_navigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, direction, currentUrl = candidateContext->currentUrl(),
            candidateSource = candidateContext->source()](
            std::vector<ImageNavigationCandidate> candidates) mutable {
            finishNavigation(
                std::move(candidates), direction, currentUrl, std::move(candidateSource));
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelNavigation() { m_navigationListerJob.cancel(); }

void ImageNavigationService::finishNavigation(std::vector<ImageNavigationCandidate> candidates,
    NavigationDirection direction, const QUrl &currentUrl, ImageCandidateListSource candidateSource)
{
    const std::optional<QUrl> targetUrl
        = adjacentImageNavigationUrl(candidates, currentUrl, direction);
    if (!targetUrl.has_value()) {
        return;
    }

    setPageNavigationUrls(
        imageNavigationCandidateUrls(candidates), *targetUrl, std::move(candidateSource));
    invokeIfSet(m_callbacks.openUrl, *targetUrl);
}

void ImageNavigationService::openAdjacentContainer(
    const QUrl &currentContainerUrl, NavigationDirection direction)
{
    if (currentContainerUrl.isEmpty()) {
        return;
    }

    const QUrl parentUrl = parentUrlForContainerNavigation(currentContainerUrl);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();
    cancelContainerNavigation();

    m_containerNavigationListerJob = m_candidateRepository.loadContainers(
        this, parentUrl,
        [this, direction, currentContainerUrl](
            std::vector<ContainerNavigationCandidate> candidates) {
            finishContainerNavigation(std::move(candidates), direction, currentContainerUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelContainerNavigation()
{
    m_containerNavigationListerJob.cancel();
    m_containerNavigationListJob.cancel();
}

void ImageNavigationService::finishContainerNavigation(
    std::vector<ContainerNavigationCandidate> candidates, NavigationDirection direction,
    const QUrl &currentContainerUrl)
{
    const auto target
        = adjacentContainerNavigationCandidate(candidates, currentContainerUrl, direction);
    if (!target.has_value()) {
        return;
    }

    m_containerNavigationListJob = m_candidateRepository.loadFirstImageInContainer(
        this, *target,
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            openImageFromContainerNavigation(imageUrl, containerUrl);
        },
        [this](const QUrl &containerUrl, ImageCandidateRepositoryError error,
            const QString &errorString) {
            finishContainerNavigationLoadWithError(containerUrl, error, errorString);
        });
}

void ImageNavigationService::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    invokeIfSet(m_callbacks.openContainerImage, imageUrl, containerUrl);
}

void ImageNavigationService::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    invokeIfSet(m_callbacks.containerNavigationError, containerUrl, error, errorString);
}

void ImageNavigationService::updatePageNavigation(const DisplayContext &context)
{
    cancelPageNavigationUpdate();

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateContextForDisplayContext(context);
    if (!candidateContext.has_value()) {
        clearPageNavigation();
        return;
    }

    const ImageCandidateListSource candidateSource = candidateContext->source();
    const bool canReuseKnownState = m_pageNavigationSource.has_value()
        && sameImageCandidateListSource(*m_pageNavigationSource, candidateSource)
        && !m_pageNavigation.urls.empty();
    if (canReuseKnownState) {
        setPageNavigationState(
            pageNavigationStateForCurrentUrl(m_pageNavigation, candidateContext->currentUrl()),
            true);
    } else {
        clearPageNavigation();
    }

    m_pageNavigationContext = *candidateContext;
    watchPageNavigationChanges(*candidateContext);

    m_pageNavigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, currentUrl = candidateContext->currentUrl(), candidateSource](
            std::vector<ImageNavigationCandidate> candidates) {
            setPageNavigationUrls(
                imageNavigationCandidateUrls(candidates), currentUrl, candidateSource);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelPageNavigationUpdate()
{
    m_pageNavigationListerJob.cancel();
    m_pageNavigationChangesJob.cancel();
}

void ImageNavigationService::setPageNavigationUrls(
    std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source)
{
    m_pageNavigationSource = std::move(source);
    setPageNavigationState(pageNavigationStateForUrls(std::move(urls), currentUrl));
}

void ImageNavigationService::setPageNavigationState(PageNavigationState state, bool forceNotify)
{
    if (!forceNotify && samePageNavigationState(m_pageNavigation, state)) {
        return;
    }

    m_pageNavigation = std::move(state);
    invokeIfSet(m_callbacks.pageNavigationChanged);
}

void ImageNavigationService::watchPageNavigationChanges(const ImageCandidateListContext &context)
{
    if (m_pageNavigationSource.has_value()
        && sameImageCandidateListSource(*m_pageNavigationSource, context.source())
        && m_pageNavigationChangesJob.isActive()) {
        return;
    }

    m_pageNavigationChangesJob.cancel();
    const ImageCandidateListSource source = context.source();
    m_pageNavigationChangesJob = m_candidateRepository.watchCandidateChanges(
        this, context,
        [this, source](std::vector<ImageNavigationCandidate> candidates) {
            updatePageNavigationFromChangedCandidates(std::move(candidates), source);
        },
        [](const QString &) {});
}

void ImageNavigationService::updatePageNavigationFromChangedCandidates(
    std::vector<ImageNavigationCandidate> candidates, ImageCandidateListSource source)
{
    if (!m_pageNavigationContext.has_value()
        || !sameImageCandidateListSource(m_pageNavigationContext->source(), source)) {
        return;
    }

    ImageCandidateListContext context = *m_pageNavigationContext;
    const bool currentImageRemoved
        = !imageNavigationCandidatesContainUrl(candidates, context.currentUrl());
    setPageNavigationUrls(imageNavigationCandidateUrls(candidates), context.currentUrl(), source);

    if (currentImageRemoved && !deletionInProgress()) {
        handleCurrentImageRemoved(std::move(candidates), std::move(context));
    }
}

void ImageNavigationService::handleCurrentImageRemoved(
    std::vector<ImageNavigationCandidate> candidates, ImageCandidateListContext context)
{
    const ImageRemovalFallback fallback = imageRemovalFallbackForImageContext(context);
    const std::optional<QUrl> fallbackUrl
        = imageRemovalFallbackUrl(std::move(candidates), fallback);
    invokeIfSet(m_callbacks.clearCurrentImage);
    if (fallbackUrl.has_value()) {
        invokeIfSet(m_callbacks.openUrl, *fallbackUrl);
    }
}

bool ImageNavigationService::deletionInProgress() const
{
    return m_callbacks.deletionInProgress && m_callbacks.deletionInProgress();
}

void ImageNavigationService::clearPageNavigation()
{
    m_pageNavigationChangesJob.cancel();
    m_pageNavigationContext = std::nullopt;
    m_pageNavigationSource = std::nullopt;
    setPageNavigationState(PageNavigationState {});
}
}
