// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include "async/imagecallback.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageNavigationService::ImageNavigationService(
    QObject *parent, ImageNavigationCandidateProvider candidateProvider, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
    , m_containerNavigation(this, m_candidateRepository,
          ImageContainerNavigationController::Callbacks {
              m_callbacks.navigationPlan,
          })
    , m_pageNavigation(this, m_candidateRepository,
          ImagePageNavigationController::Callbacks {
              m_callbacks.navigationPlan,
              m_callbacks.pageNavigationChanged,
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
    std::optional<ImageCandidateListContext> context, NavigationDirection direction)
{
    cancelContainerNavigation();
    m_pageNavigation.openAdjacentImage(std::move(context), direction);
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

void ImageNavigationService::updatePageNavigation(std::optional<ImageCandidateListContext> context)
{
    m_pageNavigation.update(std::move(context));
}

void ImageNavigationService::cancelPageNavigationUpdate() { m_pageNavigation.cancelUpdate(); }

void ImageNavigationService::cancelAllNavigation()
{
    cancelNavigation();
    cancelContainerNavigation();
    cancelPageNavigationUpdate();
}

void ImageNavigationService::clearPageNavigation() { m_pageNavigation.clear(); }
}
