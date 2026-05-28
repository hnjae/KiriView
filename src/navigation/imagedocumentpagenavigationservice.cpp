// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagenavigationservice.h"

#include "async/imagecallback.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentPageNavigationService::ImageDocumentPageNavigationService(
    QObject *parent, ImageDocumentPageCandidateProvider candidateProvider, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_candidateRepository(std::move(candidateProvider))
    , m_containerNavigation(this, m_candidateRepository,
          ImageContainerNavigationController::Callbacks {
              m_callbacks.navigationPlan,
          })
    , m_pageNavigation(this, m_candidateRepository,
          ImageDocumentPageNavigationController::Callbacks {
              m_callbacks.navigationPlan,
              m_callbacks.pageNavigationChanged,
              m_callbacks.deletionInProgress,
          })
{
}

int ImageDocumentPageNavigationService::currentPageNumber() const
{
    return m_pageNavigation.currentPageNumber();
}

int ImageDocumentPageNavigationService::pageCount() const { return m_pageNavigation.pageCount(); }

ImageDocumentPageNavigationSnapshot
ImageDocumentPageNavigationService::pageNavigationSnapshot() const
{
    return m_pageNavigation.snapshot();
}

std::optional<QUrl> ImageDocumentPageNavigationService::urlAtPage(int pageNumber) const
{
    return m_pageNavigation.urlAtPage(pageNumber);
}

std::optional<ImageDocumentPageTarget> ImageDocumentPageNavigationService::targetAtPage(
    int pageNumber) const
{
    return m_pageNavigation.targetAtPage(pageNumber);
}

std::optional<ImageDocumentPageTarget> ImageDocumentPageNavigationService::selectPage(
    int pageNumber)
{
    return m_pageNavigation.selectPage(pageNumber);
}

void ImageDocumentPageNavigationService::openAdjacentPage(
    std::optional<ImageDocumentPageCandidateListContext> context, NavigationDirection direction)
{
    cancelContainerNavigation();
    m_pageNavigation.openAdjacentPage(std::move(context), direction);
}

void ImageDocumentPageNavigationService::cancelNavigation() { m_pageNavigation.cancelNavigation(); }

void ImageDocumentPageNavigationService::openAdjacentContainer(
    const QUrl &currentContainerUrl, NavigationDirection direction)
{
    if (!ImageContainerNavigationController::canOpenAdjacentContainer(currentContainerUrl)) {
        return;
    }

    cancelNavigation();
    m_containerNavigation.openAdjacentContainer(currentContainerUrl, direction);
}

void ImageDocumentPageNavigationService::cancelContainerNavigation()
{
    m_containerNavigation.cancel();
}

void ImageDocumentPageNavigationService::updatePageNavigation(
    std::optional<ImageDocumentPageCandidateListContext> context)
{
    m_pageNavigation.update(std::move(context));
}

void ImageDocumentPageNavigationService::cancelPageNavigationUpdate()
{
    m_pageNavigation.cancelUpdate();
}

void ImageDocumentPageNavigationService::cancelAllNavigation()
{
    cancelNavigation();
    cancelContainerNavigation();
    cancelPageNavigationUpdate();
}

void ImageDocumentPageNavigationService::clearPageNavigation() { m_pageNavigation.clear(); }
}
