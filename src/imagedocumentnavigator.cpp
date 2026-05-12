// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigator.h"

#include "imagecallback.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentNavigator::ImageDocumentNavigator(
    ImageDocumentNavigationController &navigationController,
    ImageSpreadPresentationController &spreadController, OpenPageCallback openPage)
    : m_navigationController(navigationController)
    , m_spreadController(spreadController)
    , m_openPage(std::move(openPage))
{
}

void ImageDocumentNavigator::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentNavigator::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

void ImageDocumentNavigator::openPreviousSinglePage() { openImageAtRelativePageOffset(-1); }

void ImageDocumentNavigator::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentNavigator::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentNavigator::openNextContainer()
{
    openAdjacentContainer(NavigationDirection::Next);
}

void ImageDocumentNavigator::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationController.urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    const bool spreadTransition = m_spreadController.shouldBeginTransition(pageNumber);
    if (spreadTransition) {
        m_spreadController.beginTransition();
    }

    invokeIfSet(m_openPage, *pageUrl, spreadTransition);
}

void ImageDocumentNavigator::openAdjacentImage(NavigationDirection direction)
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

void ImageDocumentNavigator::openAdjacentContainer(NavigationDirection direction)
{
    if (direction == NavigationDirection::Previous) {
        m_navigationController.openPreviousContainer();
        return;
    }

    m_navigationController.openNextContainer();
}

void ImageDocumentNavigator::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = m_spreadController.relativePageNavigationTarget(offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}
}
