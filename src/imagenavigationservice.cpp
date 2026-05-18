// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "imagecallback.h"

#include <QString>
#include <optional>
#include <utility>

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

}

namespace KiriView {
ImageNavigationService::ImageNavigationService(
    QObject *parent, ImageNavigationCandidateProvider candidateProvider, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
    , m_containerNavigation(this, m_candidateRepository,
          ImageContainerNavigationController::Callbacks {
              m_callbacks.openContainerImage,
              m_callbacks.containerNavigationError,
          })
    , m_pageNavigation(this, m_candidateRepository,
          ImagePageNavigationController::Callbacks {
              m_callbacks.openUrl,
              m_callbacks.pageNavigationChanged,
              m_callbacks.clearCurrentImage,
              m_callbacks.deletionInProgress,
          })
{
}

int ImageNavigationService::currentPageNumber() const
{
    return m_pageNavigation.currentPageNumber();
}

int ImageNavigationService::imageCount() const { return m_pageNavigation.imageCount(); }

ImagePageNavigationSnapshot ImageNavigationService::pageNavigationSnapshot() const
{
    return m_pageNavigation.snapshot();
}

std::optional<QUrl> ImageNavigationService::urlAtPage(int pageNumber) const
{
    return m_pageNavigation.urlAtPage(pageNumber);
}

std::optional<QUrl> ImageNavigationService::selectPage(int pageNumber)
{
    return m_pageNavigation.selectPage(pageNumber);
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    cancelContainerNavigation();
    m_pageNavigation.openAdjacentImage(imageCandidateContextForDisplayContext(context), direction);
}

void ImageNavigationService::cancelNavigation() { m_pageNavigation.cancelNavigation(); }

void ImageNavigationService::openAdjacentContainer(
    const QUrl &currentContainerUrl, NavigationDirection direction)
{
    if (!ImageContainerNavigationController::canOpenAdjacentContainer(currentContainerUrl)) {
        return;
    }

    cancelNavigation();
    m_containerNavigation.openAdjacentContainer(currentContainerUrl, direction);
}

void ImageNavigationService::cancelContainerNavigation() { m_containerNavigation.cancel(); }

void ImageNavigationService::updatePageNavigation(const DisplayContext &context)
{
    m_pageNavigation.update(imageCandidateContextForDisplayContext(context));
}

void ImageNavigationService::cancelPageNavigationUpdate() { m_pageNavigation.cancelUpdate(); }

void ImageNavigationService::clearPageNavigation() { m_pageNavigation.clear(); }
}
