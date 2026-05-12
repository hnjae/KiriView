// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentnavigator.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QRectF>
#include <QString>
#include <memory>
#include <utility>

namespace KiriView {
struct ImageDocumentRuntime final {
    using RenderContextProvider = ImageDocumentController::RenderContextProvider;
    using ChangeCallback = ImageDocumentController::ChangeCallback;
    using FileDeletionFailedCallback = ImageDocumentController::FileDeletionFailedCallback;

    ImageDocumentRuntime(QObject *documentObject, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageAsyncDependencies dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback);
    ~ImageDocumentRuntime();

    void dispatchEffect(ImageDocumentEffect effect);
    void notify(ImageDocumentChange change);

    ImageDocumentState state;
    ChangeCallback changeCallback;
    std::unique_ptr<ImageDocumentDeletionController> documentDeletionController;
    std::unique_ptr<ImagePresentationController> presentationController;
    std::unique_ptr<ImageOpenController> openController;
    std::unique_ptr<ImageDocumentNavigationController> navigationController;
    std::unique_ptr<ImageDocumentPredecodeController> predecodeController;
    std::unique_ptr<ImageSpreadPresentationController> spreadController;
    std::unique_ptr<ImageDocumentLoadController> loadController;
    std::unique_ptr<ImageDocumentEffectExecutor> effectExecutor;
    std::unique_ptr<ImageDocumentNavigator> navigator;
};

ImageDocumentRuntime::ImageDocumentRuntime(QObject *documentObject,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : state([this](ImageDocumentChange change) { notify(change); })
    , changeCallback(std::move(changeCallback))
{
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
    RenderContextProvider primaryRenderContextProvider = renderContextProvider;
    RenderContextProvider spreadRenderContextProvider = std::move(renderContextProvider);
    presentationController = std::make_unique<ImagePresentationController>(documentObject,
        std::move(primaryRenderContextProvider),
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QString &errorString) {
                openController->finishAnimationLoadWithError(errorString);
            },
        });
    documentDeletionController = std::make_unique<ImageDocumentDeletionController>(documentObject,
        state, *presentationController, dependencies.candidateProvider,
        std::move(dependencies.fileOperations),
        ImageDocumentDeletionController::Callbacks {
            [this]() { notify(ImageDocumentChange::FileDeletionInProgress); },
            [this]() {
                effectExecutor->dispatchAll(loadController->clearAfterSuccessfulFileDeletion());
            },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            std::move(fileDeletionFailedCallback),
        });
    openController
        = std::make_unique<ImageOpenController>(documentObject, state, *presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return predecodeController->tryTake(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            dependencies.candidateProvider, dependencies.imageDecode);
    navigationController = std::make_unique<ImageDocumentNavigationController>(documentObject,
        state, *presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        },
        dependencies.candidateProvider);
    predecodeController = std::make_unique<ImageDocumentPredecodeController>(documentObject, state,
        *presentationController, dependencies.candidateProvider, dependencies.imageDecode);
    spreadController = std::make_unique<ImageSpreadPresentationController>(documentObject,
        std::move(spreadRenderContextProvider), state, *presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return predecodeController->tryTake(url); },
            [this]() { return navigationController->currentPageNumber(); },
            [this]() { return navigationController->imageCount(); },
            [this](int pageNumber) { return navigationController->urlAtPage(pageNumber); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    loadController = std::make_unique<ImageDocumentLoadController>(state,
        *documentDeletionController, *navigationController, *predecodeController, *openController,
        *presentationController, *spreadController);
    effectExecutor
        = std::make_unique<ImageDocumentEffectExecutor>(*navigationController, *predecodeController,
            *openController, *presentationController, *spreadController, *loadController);
    navigator = std::make_unique<ImageDocumentNavigator>(
        *navigationController, *spreadController, *loadController);
}

ImageDocumentRuntime::~ImageDocumentRuntime()
{
    if (loadController != nullptr) {
        loadController->shutdown();
    }
}

void ImageDocumentRuntime::dispatchEffect(ImageDocumentEffect effect)
{
    effectExecutor->dispatch(std::move(effect));
}

void ImageDocumentRuntime::notify(ImageDocumentChange change)
{
    spreadController->handleDocumentChange(change);
    invokeIfSet(changeCallback, change);
}

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

void ImageDocumentController::openPreviousImage() { m_runtime->navigator->openPreviousImage(); }

void ImageDocumentController::openNextImage() { m_runtime->navigator->openNextImage(); }

void ImageDocumentController::openPreviousSinglePage()
{
    m_runtime->navigator->openPreviousSinglePage();
}

void ImageDocumentController::openNextSinglePage() { m_runtime->navigator->openNextSinglePage(); }

void ImageDocumentController::openPreviousContainer()
{
    m_runtime->navigator->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_runtime->navigator->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    m_runtime->documentDeletionController->deleteDisplayedFile(mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    m_runtime->navigator->openImageAtPage(pageNumber);
}

void ImageDocumentController::resetZoom() { m_runtime->spreadController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_runtime->spreadController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_runtime->spreadController->updateRenderContext();
}

}
