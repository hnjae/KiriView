// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagedocumentruntime.h"

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

QUrl ImageDocumentController::sourceUrl() const { return m_runtime->sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    m_runtime->setSourceUrl(sourceUrl);
}

ImageDocumentStatus ImageDocumentController::status() const { return m_runtime->status(); }

bool ImageDocumentController::loading() const { return m_runtime->loading(); }

QString ImageDocumentController::errorString() const { return m_runtime->errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_runtime->windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_runtime->displayedUrl(); }

QSize ImageDocumentController::imageSize() const { return m_runtime->imageSize(); }

QSize ImageDocumentController::primaryImageSize() const { return m_runtime->primaryImageSize(); }

QSize ImageDocumentController::secondaryImageSize() const
{
    return m_runtime->secondaryImageSize();
}

QSizeF ImageDocumentController::viewportSize() const { return m_runtime->viewportSize(); }

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_runtime->setViewportSize(viewportSize);
}

QRectF ImageDocumentController::visibleItemRect() const { return m_runtime->visibleItemRect(); }

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_runtime->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const { return m_runtime->displaySize(); }

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    return m_runtime->primaryDisplaySize();
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    return m_runtime->secondaryDisplaySize();
}

qreal ImageDocumentController::zoomPercent() const { return m_runtime->zoomPercent(); }

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_runtime->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const { return m_runtime->zoomMode(); }

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    return m_runtime->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_runtime->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_runtime->steppedManualZoomPercent(stepCount);
}

int ImageDocumentController::rotationDegrees() const { return m_runtime->rotationDegrees(); }

int ImageDocumentController::currentPageNumber() const { return m_runtime->currentPageNumber(); }

int ImageDocumentController::currentLastPageNumber() const
{
    return m_runtime->currentLastPageNumber();
}

int ImageDocumentController::imageCount() const { return m_runtime->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_runtime->containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const
{
    return m_runtime->fileDeletionInProgress();
}

bool ImageDocumentController::twoPageModeEnabled() const { return m_runtime->twoPageModeEnabled(); }

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    m_runtime->setTwoPageModeEnabled(enabled);
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_runtime->twoPageModeAvailable();
}

bool ImageDocumentController::rightToLeftReadingEnabled() const
{
    return m_runtime->rightToLeftReadingEnabled();
}

void ImageDocumentController::setRightToLeftReadingEnabled(bool enabled)
{
    m_runtime->setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentController::rightToLeftReadingAvailable() const
{
    return m_runtime->rightToLeftReadingAvailable();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return m_runtime->secondaryPageVisible();
}

DisplayedImageRenderSnapshot ImageDocumentController::renderSnapshot(DisplayedPageRole role) const
{
    return m_runtime->renderSnapshot(role);
}

const QImage &ImageDocumentController::image() const { return m_runtime->image(); }

void ImageDocumentController::openPreviousImage() { m_runtime->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_runtime->openNextImage(); }

void ImageDocumentController::openPreviousSinglePage() { m_runtime->openPreviousSinglePage(); }

void ImageDocumentController::openNextSinglePage() { m_runtime->openNextSinglePage(); }

void ImageDocumentController::openPreviousContainer() { m_runtime->openPreviousContainer(); }

void ImageDocumentController::openNextContainer() { m_runtime->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    m_runtime->deleteDisplayedFile(mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_runtime->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_runtime->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_runtime->setFitMode(zoomMode);
}

void ImageDocumentController::rotateClockwise() { m_runtime->rotateClockwise(); }

void ImageDocumentController::rotateCounterclockwise() { m_runtime->rotateCounterclockwise(); }

void ImageDocumentController::updateRenderContext() { m_runtime->updateRenderContext(); }

}
