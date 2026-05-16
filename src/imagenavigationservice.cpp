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
    return m_pageNavigation.currentPageNumber();
}

int ImageNavigationService::imageCount() const { return m_pageNavigation.imageCount(); }

std::optional<QUrl> ImageNavigationService::urlAtPage(int pageNumber) const
{
    return m_pageNavigation.urlAtPage(pageNumber);
}

std::optional<QUrl> ImageNavigationService::selectPage(int pageNumber)
{
    const std::optional<QUrl> targetUrl = m_pageNavigation.selectPage(pageNumber);
    if (targetUrl.has_value()) {
        notifyPageNavigationChanged();
    }
    return targetUrl;
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    cancelContainerNavigation();
    cancelNavigation();

    if (m_pageNavigation.hasKnownSelection()) {
        const std::optional<QUrl> targetUrl = m_pageNavigation.selectAdjacentPage(direction);
        if (targetUrl.has_value()) {
            notifyPageNavigationChanged();
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

    if (m_pageNavigation.completeRefresh(
            imageNavigationCandidateUrls(candidates), *targetUrl, std::move(candidateSource))) {
        notifyPageNavigationChanged();
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

    if (m_pageNavigation.previewRefresh(*candidateContext)) {
        notifyPageNavigationChanged();
    }

    watchPageNavigationChanges(*candidateContext);

    m_pageNavigationListerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, currentUrl = candidateContext->currentUrl(),
            candidateSource = candidateContext->source()](
            std::vector<ImageNavigationCandidate> candidates) {
            if (m_pageNavigation.completeRefresh(
                    imageNavigationCandidateUrls(candidates), currentUrl, candidateSource)) {
                notifyPageNavigationChanged();
            }
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelPageNavigationUpdate()
{
    m_pageNavigationListerJob.cancel();
    m_pageNavigationChangesJob.cancel();
}

void ImageNavigationService::notifyPageNavigationChanged()
{
    invokeIfSet(m_callbacks.pageNavigationChanged);
}

void ImageNavigationService::watchPageNavigationChanges(const ImageCandidateListContext &context)
{
    if (m_pageNavigation.shouldKeepExistingWatcherFor(context)
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
    if (!m_pageNavigation.isCurrentRefreshSource(source)) {
        return;
    }

    const std::optional<ImageCandidateListContext> refreshContext
        = m_pageNavigation.refreshContext();
    if (!refreshContext.has_value()) {
        return;
    }

    ImageCandidateListContext context = *refreshContext;
    const bool currentImageRemoved
        = !imageNavigationCandidatesContainUrl(candidates, context.currentUrl());
    if (m_pageNavigation.completeRefresh(
            imageNavigationCandidateUrls(candidates), context.currentUrl(), source)) {
        notifyPageNavigationChanged();
    }

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
    if (m_pageNavigation.clear()) {
        notifyPageNavigationChanged();
    }
}
}
