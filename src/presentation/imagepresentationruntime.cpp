// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationruntime.h"

#include "presentation/imagespreadgeometry.h"
#include "rendering/imagerendering.h"
#include "rendering/imagerotation.h"

#include <utility>

namespace KiriView {
namespace {
    ImagePresentationRenderProjection hiddenProjection(DisplayedPageRole role)
    {
        ImagePresentationRenderProjection projection;
        projection.pageRole = role;
        return projection;
    }
}

ImagePresentationScopeKey ImagePresentationScopeKey::directImage(const QUrl &url)
{
    return ImagePresentationScopeKey { Kind::DirectImage, url };
}

ImagePresentationScopeKey ImagePresentationScopeKey::openedCollection(const QUrl &url)
{
    return ImagePresentationScopeKey { Kind::OpenedCollection, url };
}

bool ImagePresentationScopeKey::preservesPageNavigationZoom() const
{
    return kind == Kind::OpenedCollection && !url.isEmpty();
}

ImagePresentationRuntime::ImagePresentationRuntime(RenderContextProvider renderContextProvider)
    : m_zoomWorkflowState(std::move(renderContextProvider))
{
    m_committedSnapshot = currentSnapshot();
}

ImagePresentationSnapshot ImagePresentationRuntime::snapshot() const { return currentSnapshot(); }

ImagePresentationTransitionState ImagePresentationRuntime::transitionState() const
{
    return m_transitionState;
}

ImagePresentationMode ImagePresentationRuntime::mode() const { return m_mode; }

bool ImagePresentationRuntime::transitionInProgress() const
{
    return m_transitionState != ImagePresentationTransitionState::CommittedActive;
}

bool ImagePresentationRuntime::beginTransition()
{
    if (transitionInProgress()) {
        return false;
    }

    m_committedSnapshot = currentSnapshot();
    m_transitionState = ImagePresentationTransitionState::PreviousActive;
    return true;
}

bool ImagePresentationRuntime::showTransitionPlaceholder()
{
    if (!transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::TransitioningPlaceholder;
    return true;
}

bool ImagePresentationRuntime::finishTransition()
{
    if (!transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::CommittedActive;
    m_committedSnapshot = currentSnapshot();
    return true;
}

bool ImagePresentationRuntime::abortTransition()
{
    if (!transitionInProgress()) {
        return false;
    }

    restoreSnapshot(m_committedSnapshot);
    m_transitionState = ImagePresentationTransitionState::CommittedActive;
    return true;
}

QSize ImagePresentationRuntime::imageSize() const { return sourceImageSize(); }

QSize ImagePresentationRuntime::spreadImageSize() const
{
    return spreadImageSize(currentSnapshot());
}

QSizeF ImagePresentationRuntime::viewportSize() const
{
    return m_zoomWorkflowState.zoomState().viewportSize();
}

QSizeF ImagePresentationRuntime::displaySize() const
{
    return m_zoomWorkflowState.zoomState().displaySize();
}

QSizeF ImagePresentationRuntime::primaryDisplaySize() const
{
    return primaryDisplaySize(currentSnapshot());
}

QSizeF ImagePresentationRuntime::secondaryDisplaySize() const
{
    return secondaryDisplaySize(currentSnapshot());
}

QPointF ImagePresentationRuntime::viewportContentPosition() const
{
    return viewportFrame().contentPosition;
}

qreal ImagePresentationRuntime::zoomPercent() const
{
    return m_zoomWorkflowState.zoomState().zoomPercent();
}

ImageZoomMode ImagePresentationRuntime::zoomMode() const
{
    return m_zoomWorkflowState.zoomState().zoomMode();
}

qreal ImagePresentationRuntime::maximumManualZoomPercent() const
{
    return m_zoomWorkflowState.zoomState().maximumManualZoomPercent(
        m_zoomWorkflowState.devicePixelRatio());
}

qreal ImagePresentationRuntime::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomWorkflowState.zoomState().clampedManualZoomPercent(
        zoomPercent, m_zoomWorkflowState.devicePixelRatio());
}

qreal ImagePresentationRuntime::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomWorkflowState.zoomState().steppedManualZoomPercent(
        stepCount, m_zoomWorkflowState.devicePixelRatio());
}

int ImagePresentationRuntime::rotationDegrees() const
{
    return m_mode == ImagePresentationMode::TwoPageSpread ? 0 : m_rotationDegrees;
}

bool ImagePresentationRuntime::twoPageModeEnabled() const { return m_twoPageModeEnabled; }

bool ImagePresentationRuntime::setTwoPageModeEnabled(bool enabled)
{
    if (m_twoPageModeEnabled == enabled) {
        return false;
    }

    m_twoPageModeEnabled = enabled;
    return true;
}

bool ImagePresentationRuntime::rightToLeftReadingEnabled() const
{
    return m_rightToLeftReadingEnabled;
}

bool ImagePresentationRuntime::setRightToLeftReadingEnabled(bool enabled)
{
    if (m_rightToLeftReadingEnabled == enabled) {
        return false;
    }

    m_rightToLeftReadingEnabled = enabled;
    return true;
}

void ImagePresentationRuntime::resetRightToLeftReading() { m_rightToLeftReadingEnabled = false; }

bool ImagePresentationRuntime::secondaryPageVisible() const
{
    if (m_transitionState == ImagePresentationTransitionState::PreviousActive) {
        return m_committedSnapshot.secondaryPageVisible
            && m_committedSnapshot.mode == ImagePresentationMode::TwoPageSpread;
    }

    return m_secondaryPageVisible && m_mode == ImagePresentationMode::TwoPageSpread;
}

const ImageViewportFrame &ImagePresentationRuntime::viewportFrame() const
{
    return m_viewportCommands.projection().frame;
}

const ImageViewportProjection &ImagePresentationRuntime::viewportProjection() const
{
    return m_viewportCommands.projection();
}

ImageViewportCommand ImagePresentationRuntime::requestViewportContentPosition(
    const QPointF &contentPosition)
{
    return m_viewportCommands.requestContentPosition(contentPosition);
}

bool ImagePresentationRuntime::beginViewportCommandApplication(quint64 commandRevision)
{
    return m_viewportCommands.markCommandApplying(commandRevision);
}

bool ImagePresentationRuntime::completeViewportCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_viewportCommands.completeCommandApplication(commandRevision, actualContentPosition);
}

bool ImagePresentationRuntime::acknowledgeViewportCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_viewportCommands.acknowledgeCommand(commandRevision, actualContentPosition);
}

bool ImagePresentationRuntime::observeViewportContentPosition(
    const QPointF &contentPosition, ImageViewportObservationOrigin origin)
{
    return m_viewportCommands.observeContentPosition(contentPosition, origin);
}

ImageZoomChangeSet ImagePresentationRuntime::setViewportSize(const QSizeF &viewportSize)
{
    ImageZoomChangeSet changes
        = mutateZoomState([viewportSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setViewportSize(viewportSize, devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::Resize);
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::setZoomPercent(qreal zoomPercent)
{
    ImageZoomChangeSet changes
        = mutateZoomState([zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return {};
    }

    ImageZoomChangeSet changes
        = mutateZoomState([zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setFitMode(zoomMode, devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::resetZoom()
{
    ImageZoomChangeSet changes
        = mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.resetZoom(devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::updateRenderContext()
{
    ImageZoomChangeSet changes
        = mutateZoomState([](ImageZoomState &zoomState,
                              qreal devicePixelRatio) { zoomState.update(devicePixelRatio); },
            true);
    refreshViewportFrame(ImageViewportObservationOrigin::DevicePixelRatio);
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::setMode(ImagePresentationMode mode)
{
    if (m_mode == mode) {
        return {};
    }

    m_mode = mode;
    if (m_mode == ImagePresentationMode::TwoPageSpread) {
        m_rotationDegrees = 0;
    }

    ImageZoomChangeSet changes
        = mutateZoomState([this](ImageZoomState &zoomState, qreal devicePixelRatio) {
              const ImageZoomMode activeZoomMode = zoomState.zoomMode();
              const qreal activeZoomPercent = zoomState.zoomPercent();
              zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
              if (activeZoomMode == ImageZoomMode::Manual) {
                  zoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
                  return;
              }

              zoomState.setFitMode(activeZoomMode, devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    return changes;
}

void ImagePresentationRuntime::setSecondaryPageVisible(bool visible)
{
    m_secondaryPageVisible = visible;
}

ImageZoomChangeSet ImagePresentationRuntime::commitPrimaryPageSlot(
    const ImagePresentationPageSlotSnapshot &slot, const ImagePresentationScopeKey &scopeKey,
    ImagePresentationPrimaryChangePolicy policy)
{
    const bool imageChanged = m_primarySlot.imageRevision != slot.imageRevision;
    m_primarySlot = slot;
    m_scopeKey = scopeKey;
    if (imageChanged) {
        m_rotationDegrees = 0;
    }

    ImageZoomChangeSet changes = mutateZoomState(
        [this, scopeKey, policy](ImageZoomState &zoomState, qreal devicePixelRatio) {
            const ImageZoomMode activeZoomMode = zoomState.zoomMode();
            const qreal activeZoomPercent = zoomState.zoomPercent();
            zoomState.prepareImageContainer(
                scopeKey.preservesPageNavigationZoom() ? scopeKey.url : QUrl());
            zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
            if (policy == ImagePresentationPrimaryChangePolicy::ResetZoom) {
                zoomState.resetZoom(devicePixelRatio);
                return;
            }
            if (activeZoomMode == ImageZoomMode::Manual) {
                zoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
                return;
            }

            zoomState.setFitMode(activeZoomMode, devicePixelRatio);
        });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    if (policy == ImagePresentationPrimaryChangePolicy::PreserveZoomAndClearPan) {
        requestViewportContentPosition(QPointF());
    }
    return changes;
}

ImageZoomChangeSet ImagePresentationRuntime::commitPrimaryPageSlot(
    const ImagePresentationPageSlotSnapshot &slot, const ImagePresentationScopeKey &scopeKey)
{
    const bool preserveZoom = scopeKey.preservesPageNavigationZoom() && scopeKey == m_scopeKey;
    return commitPrimaryPageSlot(slot, scopeKey,
        preserveZoom ? ImagePresentationPrimaryChangePolicy::PreserveZoomAndClearPan
                     : ImagePresentationPrimaryChangePolicy::ResetZoom);
}

ImageZoomChangeSet ImagePresentationRuntime::commitSecondaryPageSlot(
    const ImagePresentationPageSlotSnapshot &slot)
{
    m_secondarySlot = slot;
    if (m_mode != ImagePresentationMode::TwoPageSpread) {
        return {};
    }

    ImageZoomChangeSet changes
        = mutateZoomState([this](ImageZoomState &zoomState, qreal devicePixelRatio) {
              const ImageZoomMode activeZoomMode = zoomState.zoomMode();
              const qreal activeZoomPercent = zoomState.zoomPercent();
              zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
              if (activeZoomMode == ImageZoomMode::Manual) {
                  zoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
                  return;
              }

              zoomState.setFitMode(activeZoomMode, devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
    return changes;
}

void ImagePresentationRuntime::clearPrimaryPageSlot()
{
    m_primarySlot = {};
    m_scopeKey = {};
    m_rotationDegrees = 0;
    mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.prepareImageContainer(QUrl());
        zoomState.setImageSize(QSize(), devicePixelRatio);
        zoomState.resetZoom(devicePixelRatio);
    });
    refreshViewportFrame(ImageViewportObservationOrigin::System);
}

void ImagePresentationRuntime::clearSecondaryPageSlot()
{
    m_secondarySlot = {};
    m_secondaryPageVisible = false;
    if (m_mode == ImagePresentationMode::TwoPageSpread) {
        setMode(ImagePresentationMode::SinglePage);
    }
}

ImagePresentationRotationChange ImagePresentationRuntime::resetRotationChange()
{
    if (m_rotationDegrees == 0) {
        return {};
    }

    m_rotationDegrees = 0;
    ImageZoomChangeSet changes
        = mutateZoomState([this](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::Rotation);
    return ImagePresentationRotationChange { true, changes };
}

ImagePresentationRotationChange ImagePresentationRuntime::rotateClockwiseChange()
{
    if (m_mode == ImagePresentationMode::TwoPageSpread || !m_primarySlot.hasImage) {
        return {};
    }

    m_rotationDegrees = imageRotationClockwise(m_rotationDegrees);
    ImageZoomChangeSet changes
        = mutateZoomState([this](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::Rotation);
    return ImagePresentationRotationChange { true, changes };
}

ImagePresentationRotationChange ImagePresentationRuntime::rotateCounterclockwiseChange()
{
    if (m_mode == ImagePresentationMode::TwoPageSpread || !m_primarySlot.hasImage) {
        return {};
    }

    m_rotationDegrees = imageRotationCounterclockwise(m_rotationDegrees);
    ImageZoomChangeSet changes
        = mutateZoomState([this](ImageZoomState &zoomState, qreal devicePixelRatio) {
              zoomState.setImageSize(sourceImageSize(), devicePixelRatio);
          });
    refreshViewportFrame(ImageViewportObservationOrigin::Rotation);
    return ImagePresentationRotationChange { true, changes };
}

bool ImagePresentationRuntime::resetRotation() { return resetRotationChange().changed; }

bool ImagePresentationRuntime::rotateClockwise() { return rotateClockwiseChange().changed; }

bool ImagePresentationRuntime::rotateCounterclockwise()
{
    return rotateCounterclockwiseChange().changed;
}

ImageDocumentRenderContext ImagePresentationRuntime::renderContext() const
{
    return m_zoomWorkflowState.renderContext();
}

ImageFirstDisplayDecodeContext ImagePresentationRuntime::firstDisplayDecodeContext() const
{
    const ImageDocumentRenderContext context = renderContext();
    return imageFirstDisplayDecodeContext(viewportSize(), context.devicePixelRatio);
}

ImagePresentationRenderProjection ImagePresentationRuntime::renderProjection(
    DisplayedPageRole role) const
{
    if (m_transitionState == ImagePresentationTransitionState::TransitioningPlaceholder) {
        return hiddenProjection(role);
    }

    if (m_transitionState == ImagePresentationTransitionState::PreviousActive) {
        return renderProjection(m_committedSnapshot, role);
    }

    return renderProjection(currentSnapshot(), role);
}

ImagePresentationSnapshot ImagePresentationRuntime::currentSnapshot() const
{
    return ImagePresentationSnapshot {
        m_mode,
        m_rightToLeftReadingEnabled,
        m_transitionState,
        m_scopeKey,
        m_primarySlot,
        m_secondarySlot,
        m_twoPageModeEnabled,
        m_secondaryPageVisible,
        m_zoomWorkflowState.zoomState().snapshot(),
        m_viewportCommands.projection(),
        m_rotationDegrees,
    };
}

QSize ImagePresentationRuntime::sourceImageSize() const
{
    return sourceImageSize(currentSnapshot());
}

QSize ImagePresentationRuntime::sourceImageSize(const ImagePresentationSnapshot &snapshot) const
{
    return snapshot.mode == ImagePresentationMode::TwoPageSpread
        ? spreadImageSize(snapshot)
        : logicalSinglePageImageSize(snapshot);
}

QSize ImagePresentationRuntime::logicalSinglePageImageSize() const
{
    return logicalSinglePageImageSize(currentSnapshot());
}

QSize ImagePresentationRuntime::logicalSinglePageImageSize(
    const ImagePresentationSnapshot &snapshot) const
{
    return rotatedImageSize(snapshot.primary.imageSize, snapshot.rotationDegrees);
}

QSize ImagePresentationRuntime::spreadImageSize(const ImagePresentationSnapshot &snapshot) const
{
    return imageSpreadImageSize(snapshot.primary.imageSize, snapshot.secondary.imageSize);
}

QSizeF ImagePresentationRuntime::displaySize(const ImagePresentationSnapshot &snapshot) const
{
    return snapshot.zoom.displaySize;
}

QSizeF ImagePresentationRuntime::primaryDisplaySize(const ImagePresentationSnapshot &snapshot) const
{
    if (snapshot.mode == ImagePresentationMode::SinglePage) {
        return displaySize(snapshot);
    }

    return imageSpreadScaledPageDisplaySize(
        snapshot.primary.imageSize, spreadImageSize(snapshot), displaySize(snapshot));
}

QSizeF ImagePresentationRuntime::secondaryDisplaySize(
    const ImagePresentationSnapshot &snapshot) const
{
    if (snapshot.mode == ImagePresentationMode::SinglePage) {
        return {};
    }

    return imageSpreadScaledPageDisplaySize(
        snapshot.secondary.imageSize, spreadImageSize(snapshot), displaySize(snapshot));
}

QRectF ImagePresentationRuntime::primaryPageRect(const ImagePresentationSnapshot &snapshot) const
{
    return imageSpreadPrimaryPageRect(primaryDisplaySize(snapshot), secondaryDisplaySize(snapshot),
        displaySize(snapshot), snapshot.rightToLeftReading);
}

QRectF ImagePresentationRuntime::secondaryPageRect(const ImagePresentationSnapshot &snapshot) const
{
    return imageSpreadSecondaryPageRect(primaryDisplaySize(snapshot),
        secondaryDisplaySize(snapshot), displaySize(snapshot), snapshot.rightToLeftReading);
}

ImageZoomChangeSet ImagePresentationRuntime::mutateZoomState(
    const ZoomStateMutation &mutation, bool forceTileRefresh)
{
    return m_zoomWorkflowState.mutate(mutation, forceTileRefresh).changes;
}

void ImagePresentationRuntime::refreshViewportFrame(ImageViewportObservationOrigin origin)
{
    m_viewportCommands.setGeometry(viewportSize(), displaySize(), origin);
}

ImagePresentationRenderProjection ImagePresentationRuntime::renderProjection(
    const ImagePresentationSnapshot &snapshot, DisplayedPageRole role) const
{
    const ImageDocumentRenderContext context = renderContext();
    const bool secondary = role == DisplayedPageRole::Secondary;
    const ImagePresentationPageSlotSnapshot &slot
        = secondary ? snapshot.secondary : snapshot.primary;
    if (!slot.hasImage) {
        return hiddenProjection(role);
    }
    if (secondary
        && (snapshot.mode == ImagePresentationMode::SinglePage || !snapshot.secondaryPageVisible)) {
        return hiddenProjection(role);
    }

    if (snapshot.mode == ImagePresentationMode::SinglePage) {
        return ImagePresentationRenderProjection {
            !secondary,
            role,
            slot.surface,
            slot.imageRevision,
            logicalSinglePageImageSize(snapshot),
            snapshot.zoom.displaySize,
            !secondary ? snapshot.viewport.frame.visibleItemRect : QRectF(),
            context.devicePixelRatio,
            context.maximumTextureSize,
            context.generation,
            snapshot.rotationDegrees,
        };
    }

    const QSizeF pageDisplaySize
        = secondary ? secondaryDisplaySize(snapshot) : primaryDisplaySize(snapshot);
    const QRectF pageVisibleRect
        = imageSpreadVisiblePageRect(snapshot.viewport.frame.visibleItemRect,
            secondary ? secondaryPageRect(snapshot) : primaryPageRect(snapshot));

    return ImagePresentationRenderProjection {
        true,
        role,
        slot.surface,
        slot.imageRevision,
        slot.imageSize,
        pageDisplaySize,
        pageVisibleRect,
        context.devicePixelRatio,
        context.maximumTextureSize,
        context.generation,
        0,
    };
}

void ImagePresentationRuntime::restoreSnapshot(const ImagePresentationSnapshot &snapshot)
{
    m_mode = snapshot.mode;
    m_rightToLeftReadingEnabled = snapshot.rightToLeftReading;
    m_scopeKey = snapshot.scopeKey;
    m_primarySlot = snapshot.primary;
    m_secondarySlot = snapshot.secondary;
    m_twoPageModeEnabled = snapshot.twoPageModeEnabled;
    m_secondaryPageVisible = snapshot.secondaryPageVisible;
    m_rotationDegrees = snapshot.rotationDegrees;
    m_zoomWorkflowState.clear();
    mutateZoomState([snapshot](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.prepareImageContainer(snapshot.zoom.containerUrl);
        zoomState.setViewportSize(snapshot.zoom.viewportSize, devicePixelRatio);
        zoomState.setImageSize(snapshot.zoom.imageSize, devicePixelRatio);
        if (snapshot.zoom.zoomMode == ImageZoomMode::Manual) {
            zoomState.setManualZoomPercent(snapshot.zoom.zoomPercent, devicePixelRatio);
            return;
        }
        zoomState.setFitMode(snapshot.zoom.zoomMode, devicePixelRatio);
    });
    m_viewportCommands = {};
    m_viewportCommands.setGeometry(
        snapshot.zoom.viewportSize, snapshot.zoom.displaySize, snapshot.viewport.observationOrigin);
    m_viewportCommands.observeContentPosition(
        snapshot.viewport.frame.contentPosition, snapshot.viewport.observationOrigin);
}
}
