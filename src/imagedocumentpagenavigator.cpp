// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagenavigator.h"

#include "imagecallback.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentPageNavigator::ImageDocumentPageNavigator(
    ImageDocumentNavigationController &navigationController,
    ImageSpreadPresentationController &spreadController, OpenPageCallback openPage)
    : m_navigationController(navigationController)
    , m_spreadController(spreadController)
    , m_openPage(std::move(openPage))
{
}

void ImageDocumentPageNavigator::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentPageNavigator::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

void ImageDocumentPageNavigator::openPreviousSinglePage() { openImageAtRelativePageOffset(-1); }

void ImageDocumentPageNavigator::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentPageNavigator::openImageAtPage(int pageNumber)
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

void ImageDocumentPageNavigator::openAdjacentImage(NavigationDirection direction)
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

void ImageDocumentPageNavigator::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = m_spreadController.relativePageNavigationTarget(offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}
}
