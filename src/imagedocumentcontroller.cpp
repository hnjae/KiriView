// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagecontainer.h"
#include "imagedocumentnavigationcontroller.h"
#include "imageopencontroller.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"

#include <QCoreApplication>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::windowTitleFileNameForDisplayedUrl;

QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}

namespace KiriView {
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : QObject(parent)
    , m_changeCallback(std::move(changeCallback))
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    m_presentationController = std::make_unique<ImagePresentationController>(
        this, std::move(renderContextProvider),
        [this](ImageDocumentChange change) { notify(change); },
        [this](const QString &errorString) { finishWithAnimationError(errorString); });
    m_openController = std::make_unique<ImageOpenController>(
        this, m_state, *m_presentationController,
        [this](const QUrl &url) { return takePredecodedImage(url); }, [this]() { clearImage(); },
        [this]() { m_navigationController->updatePageNavigation(); },
        [this]() { scheduleAdjacentImagePredecode(); });
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(
        this, m_state, *m_presentationController,
        [this](ImageDocumentChange change) { notify(change); },
        [this](const QUrl &url) { setSourceUrl(url); },
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            setSourceUrlForLoad(imageUrl, containerUrl);
        },
        [this](const QUrl &containerUrl) {
            m_openController->finishContainerNavigationWithEmptyContainer(containerUrl);
        },
        [this](const QUrl &containerUrl, const QString &errorString) {
            m_openController->finishContainerNavigationLoadWithError(containerUrl, errorString);
        });
    m_predecodeCoordinator = std::make_unique<ImagePredecodeCoordinator>(this);
}

ImageDocumentController::~ImageDocumentController()
{
    m_presentationController->stopAnimation();
    cancelPredecode();
    m_navigationController->cancelPageNavigationUpdate();
    m_navigationController->cancelContainerNavigation();
    m_navigationController->cancelNavigation();
    m_openController->cancel();
}

QUrl ImageDocumentController::sourceUrl() const { return m_state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    setSourceUrlForLoad(sourceUrl, QUrl());
}

ImageDocumentStatus ImageDocumentController::status() const { return m_state.status(); }

bool ImageDocumentController::loading() const { return m_state.loading(); }

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QSize ImageDocumentController::imageSize() const { return m_presentationController->imageSize(); }

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
}

QSizeF ImageDocumentController::displaySize() const
{
    return m_presentationController->displaySize();
}

qreal ImageDocumentController::zoomPercent() const
{
    return m_presentationController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_presentationController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    return m_presentationController->zoomMode();
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationController->currentPageNumber();
}

int ImageDocumentController::imageCount() const { return m_navigationController->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision() const
{
    return m_presentationController->imageRevision();
}

void ImageDocumentController::openPreviousImage() { m_navigationController->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_navigationController->openNextImage(); }

void ImageDocumentController::openPreviousContainer()
{
    m_navigationController->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_navigationController->openNextContainer(); }

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_navigationController->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_presentationController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_presentationController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
}

void ImageDocumentController::setSourceUrlForLoad(
    const QUrl &sourceUrl, const QUrl &containerNavigationUrl)
{
    if (m_state.sourceUrl() == sourceUrl) {
        m_state.clearLoadingContainerNavigationUrl();
        if (!containerNavigationUrl.isEmpty()) {
            m_state.setContainerNavigationUrl(containerNavigationUrl);
        }
        return;
    }

    m_navigationController->cancelNavigation();
    m_navigationController->cancelContainerNavigation();
    cancelPredecode();
    m_state.setLoadingContainerNavigationUrl(containerNavigationUrl);
    m_state.setSourceUrl(sourceUrl);
    m_openController->open();
}

void ImageDocumentController::scheduleAdjacentImagePredecode()
{
    if (!m_presentationController->hasImage() || m_state.displayedUrl().isEmpty()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(ImagePredecodeCoordinator::Context { m_state.displayedUrl(),
        m_state.displayedComicBookRootUrl(), m_presentationController->isPredecodeCacheable(),
        m_presentationController->image() });
}

void ImageDocumentController::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<PredecodedImage> ImageDocumentController::takePredecodedImage(const QUrl &url) const
{
    QImage image;
    QUrl comicBookRootUrl;
    if (m_predecodeCoordinator == nullptr
        || !m_predecodeCoordinator->tryTake(url, &image, &comicBookRootUrl)) {
        return std::nullopt;
    }

    return PredecodedImage { image, comicBookRootUrl };
}

void ImageDocumentController::finishWithAnimationError(const QString &errorString)
{
    m_state.setLoading(false);
    clearImage();
    resetZoom();
    m_state.setContainerNavigationUrl(QUrl());
    m_navigationController->clearPageNavigation();
    const QString message = errorString.isEmpty()
        ? imageViewText("Could not decode the selected image animation.")
        : errorString;
    m_state.setErrorString(message);
    m_state.setStatus(ImageDocumentStatus::Error);
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    m_navigationController->cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController->clearImage();
    m_state.setWindowTitleFileName(windowTitleFileNameForDisplayedUrl(
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl()));
    m_navigationController->clearPageNavigation();
}
}
