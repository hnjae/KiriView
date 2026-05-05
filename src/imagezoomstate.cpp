// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagezoomstate.h"

#include "imageurl.h"
#include "kiriview/src/imagezoomstate.cxx.h"

namespace {
KiriView::RustZoomSize rustZoomSize(const QSize &size)
{
    return KiriView::RustZoomSize { size.width(), size.height() };
}

KiriView::RustZoomSizeF rustZoomSizeF(const QSizeF &size)
{
    return KiriView::RustZoomSizeF { size.width(), size.height() };
}

QSize qtSize(const KiriView::RustZoomSize &size) { return QSize(size.width, size.height); }

QSizeF qtSizeF(const KiriView::RustZoomSizeF &size) { return QSizeF(size.width, size.height); }

QSizeF qtSizeF(qreal width, qreal height) { return QSizeF(width, height); }

KiriView::RustImageZoomMode rustZoomMode(KiriView::ImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case KiriView::ImageZoomMode::Fit:
        return KiriView::RustImageZoomMode::Fit;
    case KiriView::ImageZoomMode::FitHeight:
        return KiriView::RustImageZoomMode::FitHeight;
    case KiriView::ImageZoomMode::FitWidth:
        return KiriView::RustImageZoomMode::FitWidth;
    case KiriView::ImageZoomMode::Manual:
        return KiriView::RustImageZoomMode::Manual;
    }

    return KiriView::RustImageZoomMode::Fit;
}

KiriView::ImageZoomMode qtZoomMode(KiriView::RustImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case KiriView::RustImageZoomMode::Fit:
        return KiriView::ImageZoomMode::Fit;
    case KiriView::RustImageZoomMode::FitHeight:
        return KiriView::ImageZoomMode::FitHeight;
    case KiriView::RustImageZoomMode::FitWidth:
        return KiriView::ImageZoomMode::FitWidth;
    case KiriView::RustImageZoomMode::Manual:
        return KiriView::ImageZoomMode::Manual;
    }

    return KiriView::ImageZoomMode::Fit;
}
}

namespace KiriView {
bool imageZoomApproximatelyEqual(qreal left, qreal right)
{
    return rustImageZoomApproximatelyEqual(left, right);
}

bool imageZoomApproximatelyEqual(const QSizeF &left, const QSizeF &right)
{
    return rustImageZoomSizeApproximatelyEqual(rustZoomSizeF(left), rustZoomSizeF(right));
}

qreal ImageZoomState::minimumManualZoomPercent() { return rustImageZoomMinimumManualZoomPercent(); }

qreal ImageZoomState::maximumManualZoomPercent() { return rustImageZoomMaximumManualZoomPercent(); }

int ImageZoomState::manualZoomStepPercent() { return rustImageZoomManualZoomStepPercent(); }

const QSize &ImageZoomState::imageSize() const { return m_imageSize; }

const QSizeF &ImageZoomState::viewportSize() const { return m_viewportSize; }

const QSizeF &ImageZoomState::displaySize() const { return m_displaySize; }

qreal ImageZoomState::zoomPercent() const { return m_zoomPercent; }

ImageZoomMode ImageZoomState::zoomMode() const { return m_zoomMode; }

const QUrl &ImageZoomState::containerUrl() const { return m_containerUrl; }

ImageZoomSnapshot ImageZoomState::snapshot() const
{
    return ImageZoomSnapshot { m_imageSize, m_viewportSize, m_displaySize, m_zoomPercent,
        m_zoomMode, m_containerUrl };
}

RustImageZoomState ImageZoomState::rustState() const
{
    return RustImageZoomState { m_imageSize.width(), m_imageSize.height(), m_viewportSize.width(),
        m_viewportSize.height(), m_displaySize.width(), m_displaySize.height(), m_zoomPercent,
        rustZoomMode(m_zoomMode) };
}

void ImageZoomState::applyRustState(const RustImageZoomState &state)
{
    m_imageSize = qtSize(RustZoomSize { state.image_width, state.image_height });
    m_viewportSize = qtSizeF(state.viewport_width, state.viewport_height);
    m_displaySize = qtSizeF(state.display_width, state.display_height);
    m_zoomPercent = state.zoom_percent;
    m_zoomMode = qtZoomMode(state.zoom_mode);
}

bool ImageZoomState::setViewportSize(const QSizeF &viewportSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetViewportSize(rustState(), rustZoomSizeF(viewportSize), devicePixelRatio);
    applyRustState(change.state);
    return change.changed;
}

bool ImageZoomState::setImageSize(const QSize &imageSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetImageSize(rustState(), rustZoomSize(imageSize), devicePixelRatio);
    applyRustState(change.state);
    return change.changed;
}

bool ImageZoomState::setManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetManualZoomPercent(rustState(), zoomPercent, devicePixelRatio);
    applyRustState(change.state);
    return change.changed;
}

bool ImageZoomState::setFitMode(ImageZoomMode zoomMode, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetFitMode(rustState(), rustZoomMode(zoomMode), devicePixelRatio);
    applyRustState(change.state);
    return change.changed;
}

void ImageZoomState::resetZoom(qreal devicePixelRatio)
{
    applyRustState(rustImageZoomResetZoom(rustState(), devicePixelRatio));
}

void ImageZoomState::prepareImageContainer(const QUrl &containerUrl)
{
    applyRustState(rustImageZoomPrepareImageContainer(
        rustState(), sameContainerNavigationUrl(containerUrl, m_containerUrl)));
    m_containerUrl = containerUrl;
}

LoadedImageZoom ImageZoomState::loadedImageZoom(
    const QUrl &containerUrl, const QSize &imageSize, qreal devicePixelRatio) const
{
    const RustLoadedImageZoom zoom = rustImageZoomLoadedImageZoom(rustState(),
        sameContainerNavigationUrl(containerUrl, m_containerUrl), rustZoomSize(imageSize),
        devicePixelRatio);
    return LoadedImageZoom { qtZoomMode(zoom.zoom_mode), zoom.zoom_percent,
        qtSizeF(zoom.display_width, zoom.display_height) };
}

void ImageZoomState::setLoadedSvgZoom(const QUrl &containerUrl, const LoadedImageZoom &zoom)
{
    applyRustState(rustImageZoomSetLoadedSvgZoom(rustState(),
        RustLoadedImageZoom { rustZoomMode(zoom.zoomMode), zoom.zoomPercent,
            zoom.displaySize.width(), zoom.displaySize.height() }));
    m_containerUrl = containerUrl;
}

void ImageZoomState::clearContainer() { m_containerUrl = QUrl(); }

void ImageZoomState::update(qreal devicePixelRatio)
{
    applyRustState(rustImageZoomUpdate(rustState(), devicePixelRatio));
}

qreal ImageZoomState::fitZoomPercent(ImageZoomMode zoomMode, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercent(rustState(), rustZoomMode(zoomMode), devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercentForImageSize(
    ImageZoomMode zoomMode, const QSize &imageSize, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercentForImageSize(
        rustState(), rustZoomMode(zoomMode), rustZoomSize(imageSize), devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return displaySizeForZoomPercent(zoomPercent, m_imageSize, devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(
    qreal zoomPercent, const QSize &imageSize, qreal devicePixelRatio) const
{
    return qtSizeF(rustImageZoomDisplaySizeForZoomPercent(
        rustZoomSize(imageSize), zoomPercent, devicePixelRatio));
}
}
