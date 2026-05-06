// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imagecallback.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <QString>
#include <algorithm>
#include <iterator>
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

    if (!setKnownPageNavigationCurrentUrl(candidateContext->currentUrl())) {
        setFallbackPageNavigationUrl(candidateContext->currentUrl());
    }

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

    PageNavigationState state = m_pageNavigation;
    state.currentIndex = currentPageIndex;
    setPageNavigationState(std::move(state));
    return true;
}

void ImageNavigationService::clearPageNavigation()
{
    setPageNavigationState(PageNavigationState {});
}
}
