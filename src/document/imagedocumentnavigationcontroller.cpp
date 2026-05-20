// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "navigation/imagenavigationservice.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
KiriView::ImageNavigationService::DisplayContext navigationDisplayContext(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImagePresentationController &presentationController)
{
    return KiriView::ImageNavigationService::DisplayContext { presentationController.hasImage(),
        state.displayedImageLocation() };
}
}

namespace KiriView {
ImageDocumentNavigationController::ImageDocumentNavigationController(ImageDocumentState &state,
    ImagePresentationController &presentationController, ImageNavigationService &navigationService,
    ImageSpreadPresentationController &spreadController, EffectCallback effectCallback)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_navigationService(navigationService)
    , m_spreadController(spreadController)
    , m_effectCallback(std::move(effectCallback))
{
}

int ImageDocumentNavigationController::currentPageNumber() const
{
    return m_navigationService.currentPageNumber();
}

int ImageDocumentNavigationController::imageCount() const
{
    return m_navigationService.imageCount();
}

ImagePageNavigationSnapshot ImageDocumentNavigationController::pageNavigationSnapshot() const
{
    return m_navigationService.pageNavigationSnapshot();
}

void ImageDocumentNavigationController::openAdjacentImage(NavigationDirection direction)
{
    const ImageSpreadPageNavigationTarget target
        = m_spreadController.imageNavigationTarget(direction);
    if (!target.handledBySpread) {
        m_navigationService.openAdjacentImage(
            navigationDisplayContext(m_state, m_presentationController), direction);
        return;
    }

    if (target.pageNumber <= 0) {
        return;
    }

    openImageAtPage(target.pageNumber);
}

void ImageDocumentNavigationController::openAdjacentContainer(NavigationDirection direction)
{
    m_navigationService.openAdjacentContainer(m_state.containerNavigationUrl(), direction);
}

void ImageDocumentNavigationController::openImageAtPage(int pageNumber)
{
    const bool spreadTransition = m_spreadController.shouldBeginTransition(pageNumber);
    const std::optional<QUrl> pageUrl = m_navigationService.selectPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    if (spreadTransition) {
        m_spreadController.beginTransition();
    }

    invokeIfSet(
        m_effectCallback, ImageDocumentEffect::pageNavigationSelected(*pageUrl, spreadTransition));
}

void ImageDocumentNavigationController::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = m_spreadController.relativePageNavigationTarget(offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}

void ImageDocumentNavigationController::updatePageNavigation()
{
    m_navigationService.updatePageNavigation(
        navigationDisplayContext(m_state, m_presentationController));
}

void ImageDocumentNavigationController::cancelNavigation()
{
    m_navigationService.cancelNavigation();
}

void ImageDocumentNavigationController::cancelContainerNavigation()
{
    m_navigationService.cancelContainerNavigation();
}

void ImageDocumentNavigationController::cancelPageNavigationUpdate()
{
    m_navigationService.cancelPageNavigationUpdate();
}

void ImageDocumentNavigationController::cancelAllNavigation()
{
    m_navigationService.cancelAllNavigation();
}

void ImageDocumentNavigationController::clearPageNavigation()
{
    m_navigationService.clearPageNavigation();
}
}
