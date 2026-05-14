// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcoordinator.h"

#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <optional>

namespace KiriView {
ImageDocumentNavigationCoordinator::ImageDocumentNavigationCoordinator(
    ImageDocumentNavigationController &navigationController,
    ImageSpreadPresentationController &spreadController,
    ImageDocumentLoadController &loadController)
    : m_navigationController(navigationController)
    , m_spreadController(spreadController)
    , m_loadController(loadController)
{
}

void ImageDocumentNavigationCoordinator::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentNavigationCoordinator::openNextImage()
{
    openAdjacentImage(NavigationDirection::Next);
}

void ImageDocumentNavigationCoordinator::openPreviousSinglePage()
{
    openImageAtRelativePageOffset(-1);
}

void ImageDocumentNavigationCoordinator::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentNavigationCoordinator::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentNavigationCoordinator::openNextContainer()
{
    openAdjacentContainer(NavigationDirection::Next);
}

void ImageDocumentNavigationCoordinator::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationController.urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    const bool spreadTransition = m_spreadController.shouldBeginTransition(pageNumber);
    if (spreadTransition) {
        m_spreadController.beginTransition();
    }

    m_loadController.loadSource(
        ImageDocumentSourceLoadRequest::fromPageNavigation(*pageUrl, spreadTransition));
}

void ImageDocumentNavigationCoordinator::openAdjacentImage(NavigationDirection direction)
{
    const ImageSpreadPageNavigationTarget target
        = m_spreadController.imageNavigationTarget(direction);
    if (!target.handledBySpread) {
        if (direction == NavigationDirection::Previous) {
            m_navigationController.openPreviousImage();
            return;
        }

        m_navigationController.openNextImage();
        return;
    }

    if (target.pageNumber <= 0) {
        return;
    }

    openImageAtPage(target.pageNumber);
}

void ImageDocumentNavigationCoordinator::openAdjacentContainer(NavigationDirection direction)
{
    if (direction == NavigationDirection::Previous) {
        m_navigationController.openPreviousContainer();
        return;
    }

    m_navigationController.openNextContainer();
}

void ImageDocumentNavigationCoordinator::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = m_spreadController.relativePageNavigationTarget(offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}
}
