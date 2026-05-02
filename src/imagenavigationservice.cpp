// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QString>
#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace {
bool samePageNavigationState(
    const KiriView::PageNavigationState &left, const KiriView::PageNavigationState &right)
{
    return left.urls == right.urls && left.currentIndex == right.currentIndex;
}
}

namespace KiriView {
ImageNavigationService::ImageNavigationService(QObject *parent)
    : QObject(parent)
    , m_candidateRepository()
{
}

ImageNavigationService::ImageNavigationService(
    QObject *parent, ImageNavigationCandidateProvider candidateProvider)
    : QObject(parent)
    , m_candidateRepository(std::move(candidateProvider))
{
}

void ImageNavigationService::setOpenUrlCallback(OpenUrlCallback callback)
{
    m_openUrl = std::move(callback);
}

void ImageNavigationService::setOpenContainerImageCallback(OpenContainerImageCallback callback)
{
    m_openContainerImage = std::move(callback);
}

void ImageNavigationService::setContainerNavigationErrorCallback(
    ContainerNavigationErrorCallback callback)
{
    m_containerNavigationError = std::move(callback);
}

void ImageNavigationService::setPageNavigationChangedCallback(
    PageNavigationChangedCallback callback)
{
    m_pageNavigationChanged = std::move(callback);
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
    if (pageNumber <= 0 || pageNumber > imageCount()) {
        return std::nullopt;
    }

    const int pageIndex = pageNumber - 1;
    if (pageIndex == m_pageNavigation.currentIndex) {
        return std::nullopt;
    }

    return m_pageNavigation.urls.at(static_cast<std::size_t>(pageIndex));
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        return;
    }

    cancelContainerNavigation();

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(
            context.displayedUrl, context.comicBookRootUrl);
    if (!candidateContext.has_value()) {
        return;
    }

    cancelNavigation();

    m_navigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, direction, currentUrl = candidateContext->currentUrl](
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

    if (m_openUrl) {
        m_openUrl(*targetUrl);
    }
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
            finishContainerNavigationLoadWithRepositoryError(containerUrl, error, errorString);
        });
}

void ImageNavigationService::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    if (m_openContainerImage) {
        m_openContainerImage(imageUrl, containerUrl);
    }
}

void ImageNavigationService::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    if (m_containerNavigationError) {
        m_containerNavigationError(containerUrl, error, errorString);
    }
}

void ImageNavigationService::finishContainerNavigationLoadWithRepositoryError(
    const QUrl &containerUrl, ImageCandidateRepositoryError error, const QString &errorString)
{
    switch (error) {
    case ImageCandidateRepositoryError::Generic:
        finishContainerNavigationLoadWithError(
            containerUrl, ContainerNavigationError::Generic, errorString);
        return;
    case ImageCandidateRepositoryError::EmptyContainer:
        finishContainerNavigationLoadWithError(
            containerUrl, ContainerNavigationError::EmptyContainer, errorString);
        return;
    case ImageCandidateRepositoryError::InvalidComicBookArchive:
        finishContainerNavigationLoadWithError(
            containerUrl, ContainerNavigationError::InvalidComicBookArchive, errorString);
        return;
    }
}

void ImageNavigationService::updatePageNavigation(const DisplayContext &context)
{
    cancelPageNavigationUpdate();

    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(
            context.displayedUrl, context.comicBookRootUrl);
    if (!candidateContext.has_value()) {
        clearPageNavigation();
        return;
    }

    if (!setKnownPageNavigationCurrentUrl(candidateContext->currentUrl)) {
        setFallbackPageNavigationUrl(candidateContext->currentUrl);
    }

    m_pageNavigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, currentUrl = candidateContext->currentUrl](
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
    if (m_pageNavigationChanged) {
        m_pageNavigationChanged();
    }
}

void ImageNavigationService::setFallbackPageNavigationUrl(const QUrl &currentUrl)
{
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setPageNavigationUrls({ normalizedImageUrl(currentUrl) }, currentUrl);
}

bool ImageNavigationService::setKnownPageNavigationCurrentUrl(const QUrl &currentUrl)
{
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        return false;
    }

    const auto current = std::find_if(m_pageNavigation.urls.cbegin(), m_pageNavigation.urls.cend(),
        [&currentUrl](const QUrl &url) { return sameNormalizedUrl(url, currentUrl); });
    if (current == m_pageNavigation.urls.cend()) {
        return false;
    }

    const int currentPageIndex
        = static_cast<int>(std::distance(m_pageNavigation.urls.cbegin(), current));
    if (m_pageNavigation.currentIndex == currentPageIndex) {
        return true;
    }

    m_pageNavigation.currentIndex = currentPageIndex;
    if (m_pageNavigationChanged) {
        m_pageNavigationChanged();
    }
    return true;
}

void ImageNavigationService::clearPageNavigation()
{
    setPageNavigationState(PageNavigationState {});
}
}
