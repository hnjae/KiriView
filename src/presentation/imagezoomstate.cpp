// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagezoomstate.h"

#include "bridge/imagezoomstateconversion.h"
#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagezoomstate.cxx.h"
#include "location/imageurl.h"

namespace kiriview {
bool imageZoomApproximatelyEqual(qreal left, qreal right)
{
    return rustImageZoomApproximatelyEqual(left, right);
}

bool imageZoomApproximatelyEqual(const QSizeF& left, const QSizeF& right)
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
    const ImageZoomSnapshot& snapshot, qreal devicePixelRatio)
{
    return rustImageZoomMaximumManualZoomPercent(
        Bridge::rustImageZoomState(snapshot), devicePixelRatio);
}

ImageZoomChangeSet ImageZoomState::changeSet(const ImageZoomSnapshot& previous,
    qreal previousDevicePixelRatio, const ImageZoomSnapshot& current, qreal currentDevicePixelRatio,
    bool forceDisplayProjectionUpdate)
{
    const RustImageZoomChangeSet changes = rustImageZoomChangeSet(
        Bridge::rustImageZoomState(previous), previousDevicePixelRatio,
        Bridge::rustImageZoomState(current), currentDevicePixelRatio, forceDisplayProjectionUpdate);
    return ImageZoomChangeSet { changes.image_size_changed, changes.viewport_size_changed,
        changes.zoom_mode_changed, changes.zoom_percent_changed, changes.display_size_changed,
        changes.maximum_manual_zoom_percent_changed, changes.display_projection_update_needed };
}

const QSize& ImageZoomState::imageSize() const { return m_imageSize; }

const QSizeF& ImageZoomState::viewportSize() const { return m_viewportSize; }

const QSizeF& ImageZoomState::displaySize() const { return m_displaySize; }

qreal ImageZoomState::zoomPercent() const { return m_zoomPercent; }

ImageZoomMode ImageZoomState::zoomMode() const { return m_zoomMode; }

ImageZoomMode ImageZoomState::fitModeSelection() const { return m_fitModeSelection; }

const QUrl& ImageZoomState::containerUrl() const { return m_containerUrl; }

ImageZoomSnapshot ImageZoomState::snapshot() const
{
    return ImageZoomSnapshot { m_imageSize, m_viewportSize, m_displaySize, m_zoomPercent,
        m_zoomMode, m_containerUrl };
}

void ImageZoomState::applySnapshot(const ImageZoomSnapshot& snapshot)
{
    m_imageSize = snapshot.imageSize;
    m_viewportSize = snapshot.viewportSize;
    m_displaySize = snapshot.displaySize;
    m_zoomPercent = snapshot.zoomPercent;
    m_zoomMode = snapshot.zoomMode;
    if (snapshot.zoomMode != ImageZoomMode::Manual) {
        m_fitModeSelection = snapshot.zoomMode;
    }
    m_containerUrl = snapshot.containerUrl;
}

bool ImageZoomState::setViewportSize(const QSizeF& viewportSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetViewportSize(Bridge::rustImageZoomState(snapshot()),
            Bridge::rustSizeF<RustZoomSizeF>(viewportSize), devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setImageSize(const QSize& imageSize, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetImageSize(Bridge::rustImageZoomState(snapshot()),
            Bridge::rustSize<RustZoomSize>(imageSize), devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change = rustImageZoomSetManualZoomPercent(
        Bridge::rustImageZoomState(snapshot()), zoomPercent, devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(change.state, m_containerUrl));
    return change.changed;
}

bool ImageZoomState::setFitMode(ImageZoomMode zoomMode, qreal devicePixelRatio)
{
    const RustImageZoomStateChange change
        = rustImageZoomSetFitMode(Bridge::rustImageZoomState(snapshot()),
            Bridge::rustImageZoomMode(zoomMode), devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(change.state, m_containerUrl));
    return change.changed;
}

void ImageZoomState::resetZoom(qreal devicePixelRatio)
{
    const RustImageZoomState state
        = rustImageZoomResetZoom(Bridge::rustImageZoomState(snapshot()), devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(state, m_containerUrl));
}

void ImageZoomState::prepareImageContainer(const QUrl& containerUrl)
{
    const RustImageZoomState state
        = rustImageZoomPrepareImageContainer(Bridge::rustImageZoomState(snapshot()),
            sameContainerNavigationUrl(containerUrl, m_containerUrl));
    applySnapshot(Bridge::imageZoomSnapshotFromRust(state, containerUrl));
}

LoadedImageZoom ImageZoomState::loadedImageZoom(
    const QUrl& containerUrl, const QSize& imageSize, qreal devicePixelRatio) const
{
    const RustLoadedImageZoom zoom
        = rustImageZoomLoadedImageZoom(Bridge::rustImageZoomState(snapshot()),
            sameContainerNavigationUrl(containerUrl, m_containerUrl),
            Bridge::rustSize<RustZoomSize>(imageSize), devicePixelRatio);
    return Bridge::loadedImageZoomFromRust(zoom);
}

void ImageZoomState::setLoadedSvgZoom(const QUrl& containerUrl, const LoadedImageZoom& zoom)
{
    const RustImageZoomState state = rustImageZoomSetLoadedSvgZoom(
        Bridge::rustImageZoomState(snapshot()), Bridge::rustLoadedImageZoom(zoom));
    applySnapshot(Bridge::imageZoomSnapshotFromRust(state, containerUrl));
}

void ImageZoomState::clearContainer() { m_containerUrl = QUrl(); }

void ImageZoomState::update(qreal devicePixelRatio)
{
    const RustImageZoomState state
        = rustImageZoomUpdate(Bridge::rustImageZoomState(snapshot()), devicePixelRatio);
    applySnapshot(Bridge::imageZoomSnapshotFromRust(state, m_containerUrl));
}

qreal ImageZoomState::maximumManualZoomPercent(qreal devicePixelRatio) const
{
    return maximumManualZoomPercent(snapshot(), devicePixelRatio);
}

qreal ImageZoomState::clampedManualZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return rustImageZoomClampedManualZoomPercent(
        Bridge::rustImageZoomState(snapshot()), zoomPercent, devicePixelRatio);
}

qreal ImageZoomState::steppedManualZoomPercent(qreal stepCount, qreal devicePixelRatio) const
{
    return rustImageZoomSteppedManualZoomPercent(
        Bridge::rustImageZoomState(snapshot()), stepCount, devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercent(ImageZoomMode zoomMode, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercent(Bridge::rustImageZoomState(snapshot()),
        Bridge::rustImageZoomMode(zoomMode), devicePixelRatio);
}

qreal ImageZoomState::fitZoomPercentForImageSize(
    ImageZoomMode zoomMode, const QSize& imageSize, qreal devicePixelRatio) const
{
    return rustImageZoomFitZoomPercentForImageSize(Bridge::rustImageZoomState(snapshot()),
        Bridge::rustImageZoomMode(zoomMode), Bridge::rustSize<RustZoomSize>(imageSize),
        devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(qreal zoomPercent, qreal devicePixelRatio) const
{
    return displaySizeForZoomPercent(zoomPercent, m_imageSize, devicePixelRatio);
}

QSizeF ImageZoomState::displaySizeForZoomPercent(
    qreal zoomPercent, const QSize& imageSize, qreal devicePixelRatio) const
{
    return Bridge::qtSizeF(rustImageZoomDisplaySizeForZoomPercent(
        Bridge::rustSize<RustZoomSize>(imageSize), zoomPercent, devicePixelRatio));
}
}
