// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagedocumentchangedispatcher.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentnavigator.h"
#include "imagedocumentpredecodecontroller.h"
#include "imageopencontroller.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QRectF>
#include <QString>
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
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
    RenderContextProvider primaryRenderContextProvider = renderContextProvider;
    RenderContextProvider spreadRenderContextProvider = std::move(renderContextProvider);
    m_presentationController = std::make_unique<ImagePresentationController>(this,
        std::move(primaryRenderContextProvider),
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QString &errorString) {
                m_openController->finishAnimationLoadWithError(errorString);
            },
        });
    m_documentDeletionController = std::make_unique<ImageDocumentDeletionController>(this, m_state,
        *m_presentationController, dependencies.candidateProvider,
        std::move(dependencies.fileOperations),
        ImageDocumentDeletionController::Callbacks {
            [this]() { notify(ImageDocumentChange::FileDeletionInProgress); },
            [this]() {
                m_effectExecutor->dispatchAll(m_loadController->clearAfterSuccessfulFileDeletion());
            },
            [this](const QUrl &url) {
                m_loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(url));
            },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                m_loadController->loadSource(
                    ImageDocumentSourceLoadRequest::fromContainerImage(imageUrl, containerUrl));
            },
            std::move(fileDeletionFailedCallback),
        });
    m_openController
        = std::make_unique<ImageOpenController>(this, m_state, *m_presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return m_predecodeController->tryTake(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            dependencies.candidateProvider, dependencies.imageDecode);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(this, m_state,
        *m_presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        },
        dependencies.candidateProvider);
    m_predecodeController = std::make_unique<ImageDocumentPredecodeController>(this, m_state,
        *m_presentationController, dependencies.candidateProvider, dependencies.imageDecode);
    m_spreadController = std::make_unique<ImageSpreadPresentationController>(this,
        std::move(spreadRenderContextProvider), m_state, *m_presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return m_predecodeController->tryTake(url); },
            [this]() { return currentPageNumber(); },
            [this]() { return imageCount(); },
            [this](int pageNumber) { return m_navigationController->urlAtPage(pageNumber); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    m_changeDispatcher = std::make_unique<ImageDocumentChangeDispatcher>(
        m_state, *m_spreadController, std::move(changeCallback));
    m_loadController = std::make_unique<ImageDocumentLoadController>(m_state,
        *m_documentDeletionController, *m_navigationController, *m_predecodeController,
        *m_openController, *m_spreadController);
    m_effectExecutor = std::make_unique<ImageDocumentEffectExecutor>(m_state,
        *m_navigationController, *m_predecodeController, *m_openController,
        *m_presentationController, *m_spreadController, *m_loadController);
    m_navigator = std::make_unique<ImageDocumentNavigator>(*m_navigationController,
        *m_spreadController, [this](const QUrl &url, bool preserveTwoPageSpreadTransition) {
            m_loadController->loadSource(ImageDocumentSourceLoadRequest::fromPageNavigation(
                url, preserveTwoPageSpreadTransition));
        });
}

ImageDocumentController::~ImageDocumentController()
{
    m_documentDeletionController->cancel();
    m_presentationController->stopAnimation();
    m_spreadController->shutdown();
    m_predecodeController->cancel();
    m_navigationController->cancelPageNavigationUpdate();
    m_navigationController->cancelContainerNavigation();
    m_navigationController->cancelNavigation();
    m_openController->cancel();
}

QUrl ImageDocumentController::sourceUrl() const { return m_state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    m_loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(sourceUrl));
}

ImageDocumentStatus ImageDocumentController::status() const
{
    return m_spreadController->status(m_state.status());
}

bool ImageDocumentController::loading() const
{
    return m_spreadController->loading(m_state.loading());
}

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const { return m_spreadController->imageSize(); }

QSize ImageDocumentController::primaryImageSize() const
{
    return m_presentationController->imageSize();
}

QSize ImageDocumentController::secondaryImageSize() const
{
    return m_spreadController->secondaryImageSize();
}

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_spreadController->setViewportSize(viewportSize);
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return m_spreadController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_spreadController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const { return m_spreadController->displaySize(); }

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    return m_spreadController->primaryDisplaySize();
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    return m_spreadController->secondaryDisplaySize();
}

qreal ImageDocumentController::zoomPercent() const { return m_spreadController->zoomPercent(); }

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_spreadController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const { return m_spreadController->zoomMode(); }

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    return m_spreadController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_spreadController->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_spreadController->steppedManualZoomPercent(stepCount);
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationController->currentPageNumber();
}

int ImageDocumentController::currentLastPageNumber() const
{
    return m_spreadController->currentLastPageNumber();
}

int ImageDocumentController::imageCount() const { return m_navigationController->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const
{
    return m_documentDeletionController->inProgress();
}

bool ImageDocumentController::twoPageModeEnabled() const
{
    return m_spreadController->twoPageModeEnabled();
}

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    m_spreadController->setTwoPageModeEnabled(enabled);
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_spreadController->twoPageModeAvailable();
}

bool ImageDocumentController::rightToLeftReadingEnabled() const
{
    return m_spreadController->rightToLeftReadingEnabled();
}

void ImageDocumentController::setRightToLeftReadingEnabled(bool enabled)
{
    m_spreadController->setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentController::rightToLeftReadingAvailable() const
{
    return m_spreadController->rightToLeftReadingAvailable();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return m_spreadController->secondaryPageVisible();
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface(
    DisplayedPageRole role) const
{
    return m_spreadController->imageSurface(role);
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision(DisplayedPageRole role) const
{
    return m_spreadController->imageRevision(role);
}

void ImageDocumentController::openPreviousImage() { m_navigator->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_navigator->openNextImage(); }

void ImageDocumentController::openPreviousSinglePage() { m_navigator->openPreviousSinglePage(); }

void ImageDocumentController::openNextSinglePage() { m_navigator->openNextSinglePage(); }

void ImageDocumentController::openPreviousContainer()
{
    m_navigationController->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_navigationController->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    m_documentDeletionController->deleteDisplayedFile(mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_navigator->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_spreadController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_spreadController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext() { m_spreadController->updateRenderContext(); }

void ImageDocumentController::dispatchEffect(ImageDocumentEffect effect)
{
    m_effectExecutor->dispatch(std::move(effect));
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    m_changeDispatcher->dispatch(change);
}

}
