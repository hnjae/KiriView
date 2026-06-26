// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagecandidatelistsource.h"
#include "navigation/imagedocumentpagenavigationservice.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QUrl>
#include <optional>
#include <utility>

namespace {
bool imageDocumentPageNavigationScopeActive(const kiriview::ImageDocumentState& state)
{
    const kiriview::DisplayedImageLocation& location = state.displayedImageLocation();
    if (!kiriview::displayedLocationIsInsideOpenedCollectionScope(location)) {
        return false;
    }

    return !state.sourceUrl().isEmpty()
        && kiriview::openedCollectionScopeContainsUrl(
            location.openedCollectionScope(), state.sourceUrl());
}

std::optional<kiriview::ImageDocumentPageCandidateListContext> navigationCandidateContext(
    const kiriview::ImageDocumentState& state,
    const kiriview::ImagePageSurfaceController& pageSurfaceController)
{
    if (!pageSurfaceController.hasImage() && !state.unsupportedOpenedCollectionVideo()) {
        return std::nullopt;
    }

    if (!imageDocumentPageNavigationScopeActive(state)) {
        return std::nullopt;
    }

    return kiriview::imageDocumentPageCandidateListContextForDisplayedImage(
        state.displayedImageLocation());
}
}

namespace kiriview {
ImageDocumentNavigationController::ImageDocumentNavigationController(ImageDocumentState& state,
    ImagePageSurfaceController& pageSurfaceController,
    ImageDocumentPageNavigationService& navigationService,
    ImageSpreadPresentationController& spreadController, RuntimePlanCallback runtimePlanCallback)
    : m_state(state)
    , m_pageSurfaceController(pageSurfaceController)
    , m_navigationService(navigationService)
    , m_spreadController(spreadController)
    , m_runtimePlanCallback(std::move(runtimePlanCallback))
{
}

int ImageDocumentNavigationController::currentPageNumber() const
{
    return m_navigationService.currentPageNumber();
}

int ImageDocumentNavigationController::pageCount() const { return m_navigationService.pageCount(); }

ImageDocumentPageNavigationSnapshot
ImageDocumentNavigationController::pageNavigationSnapshot() const
{
    return m_navigationService.pageNavigationSnapshot();
}

void ImageDocumentNavigationController::openAdjacentPage(NavigationDirection direction)
{
    const std::optional<ImageDocumentPageCandidateListContext> context
        = navigationCandidateContext(m_state, m_pageSurfaceController);
    if (!context.has_value()) {
        m_navigationService.clearPageNavigation();
        return;
    }

    const ImageSpreadPageNavigationTarget target
        = m_spreadController.imageDocumentPageNavigationTarget(direction);
    if (!target.handledBySpread) {
        m_navigationService.openAdjacentPage(context, direction);
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
    if (!navigationCandidateContext(m_state, m_pageSurfaceController).has_value()) {
        m_navigationService.clearPageNavigation();
        return;
    }

    const bool spreadTransition = m_spreadController.shouldBeginTransition(pageNumber);
    const std::optional<ImageDocumentPageTarget> target
        = m_navigationService.selectPage(pageNumber);
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
        navigationCandidateContext(m_state, m_pageSurfaceController));
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
