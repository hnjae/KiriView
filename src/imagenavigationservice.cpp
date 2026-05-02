// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imageiojobs.h"

#include <QString>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
ImageNavigationService::ImageNavigationService(QObject *parent)
    : QObject(parent)
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
    return m_currentPageIndex < 0 ? 0 : m_currentPageIndex + 1;
}

int ImageNavigationService::imageCount() const
{
    return static_cast<int>(m_pageNavigationUrls.size());
}

std::optional<QUrl> ImageNavigationService::urlAtPage(int pageNumber) const
{
    if (pageNumber <= 0 || pageNumber > imageCount()) {
        return std::nullopt;
    }

    const int pageIndex = pageNumber - 1;
    if (pageIndex == m_currentPageIndex) {
        return std::nullopt;
    }

    return m_pageNavigationUrls.at(static_cast<std::size_t>(pageIndex));
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        return;
    }

    cancelContainerNavigation();

    if (isUrlInsideArchiveRoot(context.displayedUrl, context.comicBookRootUrl)) {
        openAdjacentComicBookImage(context, direction);
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(context.displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    startDirectoryImageCandidateList(
        this, &m_navigationListerSlot, parentUrl,
        [this, direction, currentUrl](std::vector<ImageNavigationCandidate> candidates) {
            finishNavigation(std::move(candidates), direction, currentUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::openAdjacentComicBookImage(
    const DisplayContext &context, NavigationDirection direction)
{
    const QUrl currentUrl = context.displayedUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = context.comicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    startArchiveImageCandidateList(
        this, &m_navigationListSlot, archiveRootUrl,
        [this, direction, currentUrl](std::vector<ImageNavigationCandidate> candidates) {
            const std::optional<QUrl> targetUrl
                = adjacentImageNavigationUrl(candidates, currentUrl, direction);
            if (!targetUrl.has_value()) {
                return;
            }

            if (m_openUrl) {
                m_openUrl(*targetUrl);
            }
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelNavigation()
{
    m_navigationListerSlot.cancel();
    m_navigationListSlot.cancel();
}

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

    startDirectoryContainerCandidateList(
        this, &m_containerNavigationListerSlot, parentUrl,
        [this, direction, currentContainerUrl](
            std::vector<ContainerNavigationCandidate> candidates) {
            finishContainerNavigation(std::move(candidates), direction, currentContainerUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelContainerNavigation()
{
    m_containerNavigationListerSlot.cancel();
    m_containerNavigationListSlot.cancel();
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

    if (target->type == ContainerNavigationCandidateType::Directory) {
        openDirectoryContainer(target->url);
    } else {
        openComicBookContainer(target->url);
    }
}

void ImageNavigationService::openDirectoryContainer(const QUrl &containerUrl)
{
    startDirectoryImageCandidateList(
        this, &m_containerNavigationListerSlot, containerUrl,
        [this, containerUrl](std::vector<ImageNavigationCandidate> candidates) {
            finishDirectoryContainerNavigation(std::move(candidates), containerUrl);
        },
        [this, containerUrl](const QString &errorString) {
            finishDirectoryContainerNavigationWithError(containerUrl, errorString);
        });
}

void ImageNavigationService::finishDirectoryContainerNavigation(
    std::vector<ImageNavigationCandidate> candidates, const QUrl &containerUrl)
{
    if (candidates.empty()) {
        finishContainerNavigationWithEmptyContainer(containerUrl);
        return;
    }

    openImageFromContainerNavigation(candidates.front().url, containerUrl);
}

void ImageNavigationService::finishDirectoryContainerNavigationWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    finishContainerNavigationLoadWithError(
        containerUrl, ContainerNavigationError::Generic, errorString);
}

void ImageNavigationService::openComicBookContainer(const QUrl &containerUrl)
{
    const std::optional<QUrl> archiveRootUrl = comicBookArchiveRootUrl(containerUrl);
    if (!archiveRootUrl.has_value()) {
        finishContainerNavigationLoadWithError(
            containerUrl, ContainerNavigationError::InvalidComicBookArchive, QString());
        return;
    }

    startArchiveImageCandidateList(
        this, &m_containerNavigationListSlot, archiveRootUrl.value(),
        [this, containerUrl](std::vector<ImageNavigationCandidate> candidates) {
            if (candidates.empty()) {
                finishContainerNavigationWithEmptyContainer(containerUrl);
                return;
            }

            openImageFromContainerNavigation(candidates.front().url, containerUrl);
        },
        [this, containerUrl](const QString &errorString) {
            finishContainerNavigationLoadWithError(
                containerUrl, ContainerNavigationError::Generic, errorString);
        });
}

void ImageNavigationService::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    if (m_openContainerImage) {
        m_openContainerImage(imageUrl, containerUrl);
    }
}

void ImageNavigationService::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(
        containerUrl, ContainerNavigationError::EmptyContainer, QString());
}

void ImageNavigationService::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    if (m_containerNavigationError) {
        m_containerNavigationError(containerUrl, error, errorString);
    }
}

void ImageNavigationService::updatePageNavigation(const DisplayContext &context)
{
    cancelPageNavigationUpdate();

    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    const bool isComicBookPage
        = isUrlInsideArchiveRoot(context.displayedUrl, context.comicBookRootUrl);
    const QUrl currentUrl = isComicBookPage
        ? context.displayedUrl.adjusted(QUrl::NormalizePathSegments)
        : navigationSourceUrl(context.displayedUrl);
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setFallbackPageNavigationUrl(currentUrl);

    if (isComicBookPage) {
        updateComicBookPageNavigation(currentUrl, context.comicBookRootUrl);
        return;
    }

    updateFilePageNavigation(currentUrl);
}

void ImageNavigationService::updateFilePageNavigation(const QUrl &currentUrl)
{
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    startDirectoryImageCandidateList(
        this, &m_pageNavigationListerSlot, parentUrl,
        [this, currentUrl](std::vector<ImageNavigationCandidate> candidates) {
            setPageNavigationUrls(imageNavigationCandidateUrls(candidates), currentUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::updateComicBookPageNavigation(
    const QUrl &currentUrl, const QUrl &archiveRootUrl)
{
    if (archiveRootUrl.isEmpty()) {
        return;
    }

    startArchiveImageCandidateList(
        this, &m_pageNavigationListSlot, archiveRootUrl,
        [this, currentUrl](std::vector<ImageNavigationCandidate> candidates) {
            setPageNavigationUrls(imageNavigationCandidateUrls(candidates), currentUrl);
        },
        [](const QString &) {});
}

void ImageNavigationService::cancelPageNavigationUpdate()
{
    m_pageNavigationListerSlot.cancel();
    m_pageNavigationListSlot.cancel();
}

void ImageNavigationService::setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    auto state = pageNavigationStateForUrls(std::move(urls), currentUrl);

    if (m_pageNavigationUrls == state.urls && m_currentPageIndex == state.currentIndex) {
        return;
    }

    m_pageNavigationUrls = std::move(state.urls);
    m_currentPageIndex = state.currentIndex;
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

    setPageNavigationUrls({ currentUrl.adjusted(QUrl::NormalizePathSegments) }, currentUrl);
}

void ImageNavigationService::clearPageNavigation()
{
    if (m_pageNavigationUrls.empty() && m_currentPageIndex == -1) {
        return;
    }

    m_pageNavigationUrls.clear();
    m_currentPageIndex = -1;
    if (m_pageNavigationChanged) {
        m_pageNavigationChanged();
    }
}
}
