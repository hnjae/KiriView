// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagezoomstate.h"

#include "imageurl.h"
#include "kiriview/src/policy/imagezoomstate.cxx.h"
#include "qtgeometryconversion.h"

namespace {
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

KiriView::RustImageZoomState rustZoomState(const KiriView::ImageZoomSnapshot &snapshot)
{
    return KiriView::RustImageZoomState { snapshot.imageSize.width(), snapshot.imageSize.height(),
        snapshot.viewportSize.width(), snapshot.viewportSize.height(), snapshot.displaySize.width(),
        snapshot.displaySize.height(), snapshot.zoomPercent, rustZoomMode(snapshot.zoomMode) };
}

KiriView::ImageZoomSnapshot imageZoomSnapshot(
    const KiriView::RustImageZoomState &state, const QUrl &containerUrl)
{
    return KiriView::ImageZoomSnapshot {
        KiriView::Bridge::qtSize(KiriView::RustZoomSize { state.image_width, state.image_height }),
        KiriView::Bridge::qtSizeF(state.viewport_width, state.viewport_height),
        KiriView::Bridge::qtSizeF(state.display_width, state.display_height),
        state.zoom_percent,
        qtZoomMode(state.zoom_mode),
        containerUrl,
    };
}
}

namespace KiriView {
bool imageZoomApproximatelyEqual(qreal left, qreal right)
{
    return rustImageZoomApproximatelyEqual(left, right);
}

bool imageZoomApproximatelyEqual(const QSizeF &left, const QSizeF &right)
{
    return rustImageZoomSizeApproximatelyEqual(
        Bridge::rustSizeF<RustZoomSizeF>(left), Bridge::rustSizeF<RustZoomSizeF>(right));
}

qreal ImageZoomState::minimumManualZoomPercent() { return rustImageZoomMinimumManualZoomPercent(); }

int ImageZoomState::manualZoomPercentPropertyValue(qreal zoomPercent)
{
    return rustImageZoomManualZoomPercentPropertyValue(zoomPercent);
}

qreal ImageZoomState::manualZoomStepFactor() { return rustImageZoomManualZoomStepFactor(); }

qreal ImageZoomState::maximumManualZoomPercent(
    const ImageZoomSnapshot &snapshot, qreal devicePixelRatio)
{
    return rustImageZoomMaximumManualZoomPercent(rustZoomState(snapshot), devicePixelRatio);
}

ImageZoomChangeSet ImageZoomState::changeSet(const ImageZoomSnapshot &previous,
    qreal previousDevicePixelRatio, const ImageZoomSnapshot &current, qreal currentDevicePixelRatio,
    bool forceTileRefresh)
{
    const RustImageZoomChangeSet changes
        = rustImageZoomChangeSet(rustZoomState(previous), previousDevicePixelRatio,
            rustZoomState(current), currentDevicePixelRatio, forceTileRefresh);
    return ImageZoomChangeSet { changes.image_size_changed, changes.viewport_size_changed,
        changes.zoom_mode_changed, changes.zoom_percent_changed, changes.display_size_changed,
        changes.maximum_manual_zoom_percent_changed, changes.schedule_visible_tile_decode };
}

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

void ImageZoomState::applySnapshot(const ImageZoomSnapshot &snapshot)
{
    m_imageSize = snapshot.imageSize;
    m_viewportSize = snapshot.viewportSize;
    m_displaySize = snapshot.displaySize;
    m_zoomPercent = snapshot.zoomPercent;
    m_zoomMode = snapshot.zoomMode;
    m_containerUrl = snapshot.containerUrl;
}

bool ImageZoomState::setViewportSize(const QSizeF &viewportSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change = rustImageZoomSetViewportSize(rustZoomState(snapshot()),
        Bridge::rustSizeF<RustZoomSizeF>(viewportSize), devicePixelRatio);
    applySnapshot(imageZoomSnapshot(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setImageSize(const QSize &imageSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change = rustImageZoomSetImageSize(
        rustZoomState(snapshot()), Bridge::rustSize<RustZoomSize>(imageSize), devicePixelRatio);
    applySnapshot(imageZoomSnapshot(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change = rustImageZoomSetManualZoomPercent(
        rustZoomState(snapshot()), zoomPercent, devicePixelRatio);
    applySnapshot(imageZoomSnapshot(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setFitMode(ImageZoomMode zoomMode, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change = rustImageZoomSetFitMode(
        rustZoomState(snapshot()), rustZoomMode(zoomMode), devicePixelRatio);
    applySnapshot(imageZoomSnapshot(change.state, m_containerUrl));
    return change.changed;
}

void ImageZoomState::resetZoom(qreal devicePixelRatio)
{
    const RustImageZoomState state
        = rustImageZoomResetZoom(rustZoomState(snapshot()), devicePixelRatio);
    applySnapshot(imageZoomSnapshot(state, m_containerUrl));
}

void ImageZoomState::prepareImageContainer(const QUrl &containerUrl)
{
    const RustImageZoomState state = rustImageZoomPrepareImageContainer(
        rustZoomState(snapshot()), sameContainerNavigationUrl(containerUrl, m_containerUrl));
    applySnapshot(imageZoomSnapshot(state, containerUrl));
}

LoadedImageZoom ImageZoomState::loadedImageZoom(
    const QUrl &containerUrl, const QSize &imageSize, qreal devicePixelRatio) const
{
    const RustLoadedImageZoom zoom = rustImageZoomLoadedImageZoom(rustZoomState(snapshot()),
        sameContainerNavigationUrl(containerUrl, m_containerUrl),
        Bridge::rustSize<RustZoomSize>(imageSize), devicePixelRatio);
    return LoadedImageZoom { qtZoomMode(zoom.zoom_mode), zoom.zoom_percent,
        Bridge::qtSizeF(zoom.display_width, zoom.display_height) };
}

void ImageZoomState::setLoadedSvgZoom(const QUrl &containerUrl, const LoadedImageZoom &zoom)
{
    const RustImageZoomState state = rustImageZoomSetLoadedSvgZoom(rustZoomState(snapshot()),
        RustLoadedImageZoom { rustZoomMode(zoom.zoomMode), zoom.zoomPercent,
            zoom.displaySize.width(), zoom.displaySize.height() });
    applySnapshot(imageZoomSnapshot(state, containerUrl));
}

void ImageZoomState::clearContainer() { m_containerUrl = QUrl(); }

void ImageZoomState::update(qreal devicePixelRatio)
{
    const RustImageZoomState state
        = rustImageZoomUpdate(rustZoomState(snapshot()), devicePixelRatio);
    applySnapshot(imageZoomSnapshot(state, m_containerUrl));
}

qreal ImageZoomState::maximumManualZoomPercent(qreal devicePixelRatio) const
{
    return maximumManualZoomPercent(snapshot(), devicePixelRatio);
}

qreal ImageZoomState::clampedManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return rustImageZoomClampedManualZoomPercent(
        rustZoomState(snapshot()), zoomPercent, devicePixelRatio);
}

qreal ImageZoomState::steppedManualZoomPercent(qreal stepCount, qreal devicePixelRatio) const
{
    return rustImageZoomSteppedManualZoomPercent(
        rustZoomState(snapshot()), stepCount, devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercent(ImageZoomMode zoomMode, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercent(
        rustZoomState(snapshot()), rustZoomMode(zoomMode), devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercentForImageSize(
    ImageZoomMode zoomMode, const QSize &imageSize, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercentForImageSize(rustZoomState(snapshot()),
        rustZoomMode(zoomMode), Bridge::rustSize<RustZoomSize>(imageSize), devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return displaySizeForZoomPercent(zoomPercent, m_imageSize, devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(
    qreal zoomPercent, const QSize &imageSize, qreal devicePixelRatio) const
{
    return Bridge::qtSizeF(rustImageZoomDisplaySizeForZoomPercent(
        Bridge::rustSize<RustZoomSize>(imageSize), zoomPercent, devicePixelRatio));
}
}
