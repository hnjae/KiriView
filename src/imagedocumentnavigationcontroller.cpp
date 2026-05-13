// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "imagecallback.h"
#include "imagedocumentstate.h"
#include "imagenavigationservice.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"

#include <optional>
#include <utility>

namespace {
KiriView::ImageNavigationService::DisplayContext displayContext(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImagePresentationController &presentationController)
{
    return KiriView::ImageNavigationService::DisplayContext { presentationController.hasImage(),
        state.displayedImageLocation() };
}
}

namespace KiriView {
ImageDocumentNavigationController::ImageDocumentNavigationController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageDocumentNavigationController::Callbacks callbacks,
    ImageNavigationCandidateProvider candidateProvider)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
{
    auto openContainerImage = [this](const QUrl &imageUrl, const QUrl &containerUrl) {
        report(ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
    };
    auto handleError = [this](const auto &url, auto error, const auto &message) {
        if (error == ContainerNavigationError::EmptyContainer) {
            report(ImageDocumentEffect::emptyContainerSelected(url));
            return;
        }

        if (error == ContainerNavigationError::InvalidComicBookArchive) {
            report(ImageDocumentEffect::containerNavigationFailed(
                url, imageViewText("Could not open the selected comic book archive.")));
            return;
        }

        report(ImageDocumentEffect::containerNavigationFailed(url, message));
    };
    m_navigationService
        = std::make_unique<ImageNavigationService>(parent, std::move(candidateProvider),
            ImageNavigationService::Callbacks {
                [this](const QUrl &url) { report(ImageDocumentEffect::openUrl(url)); },
                std::move(openContainerImage),
                std::move(handleError),
                [this]() { notify(ImageDocumentChange::PageNavigation); },
            });
}

ImageDocumentNavigationController::~ImageDocumentNavigationController() = default;

int ImageDocumentNavigationController::currentPageNumber() const
{
    return m_navigationService->currentPageNumber();
}

int ImageDocumentNavigationController::imageCount() const
{
    return m_navigationService->imageCount();
}

std::optional<QUrl> ImageDocumentNavigationController::urlAtPage(int pageNumber) const
{
    return m_navigationService->urlAtPage(pageNumber);
}

void ImageDocumentNavigationController::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentNavigationController::openNextImage()
{
    openAdjacentImage(NavigationDirection::Next);
}

void ImageDocumentNavigationController::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentNavigationController::openNextContainer()
{
    openAdjacentContainer(NavigationDirection::Next);
}

void ImageDocumentNavigationController::updatePageNavigation()
{
    m_navigationService->updatePageNavigation(displayContext(m_state, m_presentationController));
}

void ImageDocumentNavigationController::cancelNavigation()
{
    m_navigationService->cancelNavigation();
}

void ImageDocumentNavigationController::cancelContainerNavigation()
{
    m_navigationService->cancelContainerNavigation();
}

void ImageDocumentNavigationController::cancelPageNavigationUpdate()
{
    m_navigationService->cancelPageNavigationUpdate();
}

void ImageDocumentNavigationController::clearPageNavigation()
{
    m_navigationService->clearPageNavigation();
}

void ImageDocumentNavigationController::openAdjacentImage(NavigationDirection direction)
{
    m_navigationService->openAdjacentImage(
        displayContext(m_state, m_presentationController), direction);
}

void ImageDocumentNavigationController::openAdjacentContainer(NavigationDirection direction)
{
    m_navigationService->openAdjacentContainer(m_state.containerNavigationUrl(), direction);
}

void ImageDocumentNavigationController::report(ImageDocumentEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}

void ImageDocumentNavigationController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
