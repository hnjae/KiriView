// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "navigation/imagecandidatelistsource.h"
#include "navigation/imagenavigationservice.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
std::optional<KiriView::ImageCandidateListContext> navigationCandidateContext(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImagePresentationController &presentationController)
{
    if (!presentationController.hasImage() && !state.unsupportedDocumentVideo()) {
        return std::nullopt;
    }

    return KiriView::imageCandidateListContextForDisplayedImage(state.displayedImageLocation());
}
}

namespace KiriView {
ImageDocumentNavigationController::ImageDocumentNavigationController(ImageDocumentState &state,
    ImagePresentationController &presentationController, ImageNavigationService &navigationService,
    ImageSpreadPresentationController &spreadController, RuntimePlanCallback runtimePlanCallback)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_navigationService(navigationService)
    , m_spreadController(spreadController)
    , m_runtimePlanCallback(std::move(runtimePlanCallback))
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
            navigationCandidateContext(m_state, m_presentationController), direction);
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
    const std::optional<ImageNavigationTarget> target = m_navigationService.selectPage(pageNumber);
    if (!target.has_value()) {
        return;
    }

    if (spreadTransition) {
        m_spreadController.beginTransition();
    }

    invokeIfSet(m_runtimePlanCallback,
        ImageDocumentRuntimePlan {
            LoadPageNavigationUrlOperation { *target, spreadTransition },
        });
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
        navigationCandidateContext(m_state, m_presentationController));
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
