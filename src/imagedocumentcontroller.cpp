// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentruntime.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QRectF>
#include <memory>
#include <utility>

namespace KiriView {
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : ImageDocumentController(parent, std::move(renderContextProvider), std::move(changeCallback),
          ImageAsyncDependencies {})
{
}

ImageDocumentController::ImageDocumentController(QObject *parent,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : QObject(parent)
    , m_runtime(std::make_unique<ImageDocumentRuntime>(this, std::move(renderContextProvider),
          std::move(changeCallback), std::move(dependencies),
          std::move(fileDeletionFailedCallback)))
{
}

ImageDocumentController::~ImageDocumentController() = default;

QUrl ImageDocumentController::sourceUrl() const { return m_runtime->state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    m_runtime->loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(sourceUrl));
}

ImageDocumentStatus ImageDocumentController::status() const
{
    return m_runtime->spreadController->status(m_runtime->state.status());
}

bool ImageDocumentController::loading() const
{
    return m_runtime->spreadController->loading(m_runtime->state.loading());
}

QString ImageDocumentController::errorString() const { return m_runtime->state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_runtime->state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_runtime->state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const
{
    return m_runtime->spreadController->imageSize();
}

QSize ImageDocumentController::primaryImageSize() const
{
    return m_runtime->presentationController->imageSize();
}

QSize ImageDocumentController::secondaryImageSize() const
{
    return m_runtime->spreadController->secondaryImageSize();
}

QSizeF ImageDocumentController::viewportSize() const
{
    return m_runtime->presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_runtime->spreadController->setViewportSize(viewportSize);
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return m_runtime->spreadController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_runtime->spreadController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const
{
    return m_runtime->spreadController->displaySize();
}

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    return m_runtime->spreadController->primaryDisplaySize();
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    return m_runtime->spreadController->secondaryDisplaySize();
}

qreal ImageDocumentController::zoomPercent() const
{
    return m_runtime->spreadController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_runtime->spreadController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    return m_runtime->spreadController->zoomMode();
}

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    return m_runtime->spreadController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_runtime->spreadController->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_runtime->spreadController->steppedManualZoomPercent(stepCount);
}

int ImageDocumentController::rotationDegrees() const
{
    return m_runtime->spreadController->rotationDegrees();
}

int ImageDocumentController::currentPageNumber() const
{
    return m_runtime->navigationController->currentPageNumber();
}

int ImageDocumentController::currentLastPageNumber() const
{
    return m_runtime->spreadController->currentLastPageNumber();
}

int ImageDocumentController::imageCount() const
{
    return m_runtime->navigationController->imageCount();
}

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_runtime->state.containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const
{
    return m_runtime->documentDeletionController->inProgress();
}

bool ImageDocumentController::twoPageModeEnabled() const
{
    return m_runtime->spreadController->twoPageModeEnabled();
}

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    m_runtime->spreadController->setTwoPageModeEnabled(enabled);
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_runtime->spreadController->twoPageModeAvailable();
}

bool ImageDocumentController::rightToLeftReadingEnabled() const
{
    return m_runtime->spreadController->rightToLeftReadingEnabled();
}

void ImageDocumentController::setRightToLeftReadingEnabled(bool enabled)
{
    m_runtime->spreadController->setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentController::rightToLeftReadingAvailable() const
{
    return m_runtime->spreadController->rightToLeftReadingAvailable();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return m_runtime->spreadController->secondaryPageVisible();
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface(
    DisplayedPageRole role) const
{
    return m_runtime->spreadController->imageSurface(role);
}

const QImage &ImageDocumentController::image() const
{
    return m_runtime->presentationController->image();
}

quint64 ImageDocumentController::imageRevision(DisplayedPageRole role) const
{
    return m_runtime->spreadController->imageRevision(role);
}

void ImageDocumentController::openPreviousImage() { m_runtime->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_runtime->openNextImage(); }

void ImageDocumentController::openPreviousSinglePage() { m_runtime->openPreviousSinglePage(); }

void ImageDocumentController::openNextSinglePage() { m_runtime->openNextSinglePage(); }

void ImageDocumentController::openPreviousContainer() { m_runtime->openPreviousContainer(); }

void ImageDocumentController::openNextContainer() { m_runtime->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    m_runtime->documentDeletionController->deleteDisplayedFile(mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_runtime->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_runtime->spreadController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_runtime->spreadController->setFitMode(zoomMode);
}

void ImageDocumentController::rotateClockwise() { m_runtime->spreadController->rotateClockwise(); }

void ImageDocumentController::rotateCounterclockwise()
{
    m_runtime->spreadController->rotateCounterclockwise();
}

void ImageDocumentController::updateRenderContext()
{
    m_runtime->spreadController->updateRenderContext();
}

}
