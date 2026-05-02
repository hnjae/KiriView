// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagedocumentnavigationcontroller.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace KiriView {
ImageDocumentController::ImageDocumentController(QObject *parent,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies)
    : QObject(parent)
    , m_changeCallback(std::move(changeCallback))
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    m_presentationController = std::make_unique<ImagePresentationController>(
        this, std::move(renderContextProvider),
        [this](ImageDocumentChange change) { notify(change); },
        [this](const QString &errorString) {
            dispatchEffect(ImageDocumentEffect::animationFailed(errorString));
        });
    m_openController = std::make_unique<ImageOpenController>(
        this, m_state, *m_presentationController,
        [this](const QUrl &url) { return takePredecodedImage(url); },
        [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); }, dependencies);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(
        this, m_state, *m_presentationController,
        [this](ImageDocumentChange change) { notify(change); },
        [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        dependencies.candidateProvider);
    m_predecodeCoordinator
        = std::make_unique<ImagePredecodeCoordinator>(this, dependencies.candidateProvider,
            dependencies.imageDataLoader, dependencies.imageDataDecoder);
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

void ImageDocumentController::dispatchEffect(ImageDocumentEffect effect)
{
    std::visit(
        [this](const auto &payload) {
            using Event = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Event, ClearImageEffect>) {
                clearImage();
            } else if constexpr (std::is_same_v<Event, ResetZoomEffect>) {
                resetZoom();
            } else if constexpr (std::is_same_v<Event, UpdatePageNavigationEffect>) {
                m_navigationController->updatePageNavigation();
            } else if constexpr (std::is_same_v<Event, ScheduleAdjacentImagePredecodeEffect>) {
                scheduleAdjacentImagePredecode();
            } else if constexpr (std::is_same_v<Event, OpenUrlEffect>) {
                setSourceUrl(payload.url);
            } else if constexpr (std::is_same_v<Event, ContainerImageSelectedEffect>) {
                setSourceUrlForLoad(payload.imageUrl, payload.containerUrl);
            } else if constexpr (std::is_same_v<Event, EmptyContainerSelectedEffect>) {
                m_openController->finishContainerNavigationWithEmptyContainer(payload.containerUrl);
            } else if constexpr (std::is_same_v<Event, ContainerNavigationFailedEffect>) {
                m_openController->finishContainerNavigationLoadWithError(
                    payload.containerUrl, payload.errorString);
            } else if constexpr (std::is_same_v<Event, PrepareFailedContainerEffect>) {
                m_presentationController->prepareFailedContainer(payload.containerUrl);
            } else if constexpr (std::is_same_v<Event, AnimationFailedEffect>) {
                finishWithAnimationError(payload.errorString);
            }
        },
        effect.payload);
}

void ImageDocumentController::dispatchEffects(const ImageDocumentEffects &effects)
{
    for (const ImageDocumentEffect &effect : effects.items()) {
        dispatchEffect(effect);
    }
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

    m_predecodeCoordinator->schedule(
        ImagePredecodeCoordinator::Context { m_state.displayedImageLocation(),
            m_presentationController->isPredecodeCacheable(), m_presentationController->image() });
}

void ImageDocumentController::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<PredecodedImage> ImageDocumentController::takePredecodedImage(const QUrl &url) const
{
    if (m_predecodeCoordinator == nullptr) {
        return std::nullopt;
    }

    return m_predecodeCoordinator->tryTake(url);
}

void ImageDocumentController::finishWithAnimationError(const QString &errorString)
{
    const QString message = errorString.isEmpty()
        ? imageViewText("Could not decode the selected image animation.")
        : errorString;
    const ImageDocumentEffects effects
        = ImageOpenWorkflow::finishAnimationLoadWithError(m_state, message);
    dispatchEffects(effects);
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
    m_navigationController->clearPageNavigation();
}
}
