// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationcontroller.h"

#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"

#include <QCoreApplication>
#include <optional>
#include <utility>

namespace {
QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}

namespace KiriView {
ImageDocumentNavigationController::ImageDocumentNavigationController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ChangeCallback changeCallback, OpenUrlCallback openUrl,
    OpenContainerImageCallback openContainerImage,
    ContainerNavigationEmptyCallback containerNavigationEmpty,
    ContainerNavigationErrorCallback containerNavigationError)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_changeCallback(std::move(changeCallback))
    , m_openUrl(std::move(openUrl))
    , m_openContainerImage(std::move(openContainerImage))
    , m_containerNavigationEmpty(std::move(containerNavigationEmpty))
    , m_containerNavigationError(std::move(containerNavigationError))
{
    m_navigationService = std::make_unique<ImageNavigationService>(parent);
    m_navigationService->setOpenUrlCallback([this](const QUrl &url) {
        if (m_openUrl) {
            m_openUrl(url);
        }
    });
    m_navigationService->setOpenContainerImageCallback(
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            if (m_openContainerImage) {
                m_openContainerImage(imageUrl, containerUrl);
            }
        });
    m_navigationService->setContainerNavigationErrorCallback(
        [this](
            const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString) {
            if (error == ContainerNavigationError::EmptyContainer) {
                if (m_containerNavigationEmpty) {
                    m_containerNavigationEmpty(containerUrl);
                }
                return;
            }

            if (error == ContainerNavigationError::InvalidComicBookArchive) {
                if (m_containerNavigationError) {
                    m_containerNavigationError(containerUrl,
                        imageViewText("Could not open the selected comic book archive."));
                }
                return;
            }

            if (m_containerNavigationError) {
                m_containerNavigationError(containerUrl, errorString);
            }
        });
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
    if (!pageUrl.has_value() || !m_openUrl) {
        return;
    }

    m_openUrl(*pageUrl);
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
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}
}
