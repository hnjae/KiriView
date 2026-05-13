// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imagecallback.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QString>
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
}

namespace KiriView {
ImageNavigationService::ImageNavigationService(QObject *parent)
    : ImageNavigationService(parent, Callbacks {})
{
}

ImageNavigationService::ImageNavigationService(QObject *parent, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository()
{
}

ImageNavigationService::ImageNavigationService(
    QObject *parent, ImageNavigationCandidateProvider candidateProvider)
    : ImageNavigationService(parent, std::move(candidateProvider), Callbacks {})
{
}

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
    const std::optional<std::size_t> pageIndex
        = pageNavigationTargetIndex(m_pageNavigation, pageNumber);
    if (!pageIndex.has_value()) {
        return std::nullopt;
    }

    return m_pageNavigation.urls.at(*pageIndex);
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    if (!displayContextHasNavigationSource(context)) {
        return;
    }

    cancelContainerNavigation();

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateContextForDisplayContext(context);
    if (!candidateContext.has_value()) {
        return;
    }

    cancelNavigation();

    m_navigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, direction, currentUrl = candidateContext->currentUrl()](
            std::vector<ImageNavigationCandidate> candidates) {
            finishNavigation(std::move(candidates), direction, currentUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelNavigation() { m_navigationListerJob.cancel(); }

void ImageNavigationService::finishNavigation(std::vector<ImageNavigationCandidate> candidates,
    NavigationDirection direction, const QUrl &currentUrl)
{
    const std::optional<QUrl> targetUrl
        = adjacentImageNavigationUrl(candidates, currentUrl, direction);
    if (!targetUrl.has_value()) {
        return;
    }

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

    setPageNavigationState(
        pageNavigationStateForCurrentUrl(m_pageNavigation, candidateContext->currentUrl()));

    m_pageNavigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, currentUrl = candidateContext->currentUrl()](
            std::vector<ImageNavigationCandidate> candidates) {
            setPageNavigationUrls(imageNavigationCandidateUrls(candidates), currentUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelPageNavigationUpdate() { m_pageNavigationListerJob.cancel(); }

void ImageNavigationService::setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    setPageNavigationState(pageNavigationStateForUrls(std::move(urls), currentUrl));
}

void ImageNavigationService::setPageNavigationState(PageNavigationState state)
{
    if (samePageNavigationState(m_pageNavigation, state)) {
        return;
    }

    m_pageNavigation = std::move(state);
    invokeIfSet(m_callbacks.pageNavigationChanged);
}

void ImageNavigationService::clearPageNavigation()
{
    setPageNavigationState(PageNavigationState {});
}
}
