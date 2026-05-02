// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentNavigationController::ImageDocumentNavigationController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageDocumentNavigationController::Callbacks callbacks,
    ImageNavigationCandidateProvider candidateProvider)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
{
    m_navigationService
        = std::make_unique<ImageNavigationService>(parent, std::move(candidateProvider));
    m_navigationService->setOpenUrlCallback([this](const QUrl &url) {
        if (m_callbacks.effect) {
            m_callbacks.effect(ImageDocumentEffect::openUrl(url));
        }
    });
    auto openContainerImage = [this](const QUrl &imageUrl, const QUrl &containerUrl) {
        if (m_callbacks.effect) {
            m_callbacks.effect(ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
        }
    };
    m_navigationService->setOpenContainerImageCallback(std::move(openContainerImage));
    auto handleError = [this](const auto &url, auto error, const auto &message) {
        if (error == ContainerNavigationError::EmptyContainer) {
            if (m_callbacks.effect) {
                m_callbacks.effect(ImageDocumentEffect::emptyContainerSelected(url));
            }
            return;
        }

        if (error == ContainerNavigationError::InvalidComicBookArchive) {
            if (m_callbacks.effect) {
                m_callbacks.effect(ImageDocumentEffect::containerNavigationFailed(
                    url, imageViewText("Could not open the selected comic book archive.")));
            }
            return;
        }

        if (m_callbacks.effect) {
            m_callbacks.effect(ImageDocumentEffect::containerNavigationFailed(url, message));
        }
    };
    m_navigationService->setContainerNavigationErrorCallback(std::move(handleError));
    m_navigationService->setPageNavigationChangedCallback(
        [this]() { notify(ImageDocumentChange::PageNavigation); });
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

void ImageDocumentNavigationController::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentNavigationController::openNextImage()
{
    openAdjacentImage(NavigationDirection::Next);
}

void ImageDocumentNavigationController::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationService->urlAtPage(pageNumber);
    if (!pageUrl.has_value() || !m_callbacks.effect) {
        return;
    }

    m_callbacks.effect(ImageDocumentEffect::openUrl(*pageUrl));
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
    m_navigationService->updatePageNavigation(displayContext());
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

ImageNavigationService::DisplayContext ImageDocumentNavigationController::displayContext() const
{
    return ImageNavigationService::DisplayContext { m_presentationController.hasImage(),
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl() };
}

void ImageDocumentNavigationController::openAdjacentImage(NavigationDirection direction)
{
    m_navigationService->openAdjacentImage(displayContext(), direction);
}

void ImageDocumentNavigationController::openAdjacentContainer(NavigationDirection direction)
{
    m_navigationService->openAdjacentContainer(m_state.containerNavigationUrl(), direction);
}

void ImageDocumentNavigationController::notify(ImageDocumentChange change)
{
    if (m_callbacks.change) {
        m_callbacks.change(change);
    }
}
}
