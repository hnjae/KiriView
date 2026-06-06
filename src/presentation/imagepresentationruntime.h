// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONRUNTIME_H
#define KIRIVIEW_IMAGEPRESENTATIONRUNTIME_H

#include "document/imagedocumenttypes.h"
#include "presentation/imagedisplaysourceprojection.h"
#include "presentation/imagepresentationscope.h"
#include "presentation/imagepresentationstate.h"
#include "presentation/imageviewportcommandstate.h"
#include "presentation/imagezoomworkflowstate.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagerendering.h"

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QUrl>

namespace KiriView {
enum class ImagePresentationMode {
    SinglePage,
    TwoPageSpread,
};

enum class ImagePresentationPrimaryChangePolicy {
    ResetZoom,
    PreserveZoomAndClearPan,
};

struct ImagePresentationPageSlotSnapshot {
    quint64 imageRevision = 0;
    QSize imageSize;
    bool hasImage = false;
    ImageDisplaySourceSlot displaySource;
};

struct ImagePresentationRenderProjection {
    bool visible = false;
    DisplayedPageRole pageRole = DisplayedPageRole::Primary;
    quint64 imageRevision = 0;
    QSize imageSize;
    QSizeF displaySize;
    QRectF visibleItemRect;
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = fallbackTextureSizeMax;
    quint64 renderContextGeneration = 0;
    int rotationDegrees = 0;
};

struct ImagePresentationSnapshot {
    ImagePresentationMode mode = ImagePresentationMode::SinglePage;
    bool rightToLeftReading = false;
    ImagePresentationTransitionState transitionState
        = ImagePresentationTransitionState::CommittedActive;
    ImagePresentationScopeKey scopeKey;
    ImagePresentationPageSlotSnapshot primary;
    ImagePresentationPageSlotSnapshot secondary;
    bool twoPageModeEnabled = false;
    bool secondaryPageVisible = false;
    ImageZoomSnapshot zoom;
    ImageViewportProjection viewport;
    int rotationDegrees = 0;
};

struct ImagePresentationRestorationSnapshot {
    ImagePresentationSnapshot snapshot;
};

struct ImagePresentationRotationChange {
    bool changed = false;
    ImageZoomChangeSet zoomChanges;
};

class ImagePresentationRuntime final
{
public:
    using RenderContextProvider = ImageZoomWorkflowState::RenderContextProvider;

    explicit ImagePresentationRuntime(RenderContextProvider renderContextProvider = {});

    ImagePresentationSnapshot snapshot() const;
    ImagePresentationTransitionState transitionState() const;
    ImagePresentationMode mode() const;
    bool transitionInProgress() const;
    bool beginTransition();
    bool showTransitionPlaceholder();
    bool finishTransition();
    bool abortTransition();

    QSize imageSize() const;
    QSize spreadImageSize() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    QPointF viewportContentPosition() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    bool twoPageModeEnabled() const;
    bool setTwoPageModeEnabled(bool enabled);
    bool rightToLeftReadingEnabled() const;
    bool setRightToLeftReadingEnabled(bool enabled);
    void resetRightToLeftReading();
    bool secondaryPageVisible() const;

    const ImageViewportFrame &viewportFrame() const;
    const ImageViewportProjection &viewportProjection() const;
    ImageViewportCommand requestViewportContentPosition(const QPointF &contentPosition);
    bool beginViewportCommandApplication(quint64 commandRevision);
    bool completeViewportCommandApplication(
        quint64 commandRevision, const QPointF &actualContentPosition);
    bool acknowledgeViewportCommand(quint64 commandRevision, const QPointF &actualContentPosition);
    bool observeViewportContentPosition(
        const QPointF &contentPosition, ImageViewportObservationOrigin origin);

    ImageZoomChangeSet setViewportSize(const QSizeF &viewportSize);
    ImageZoomChangeSet setZoomPercent(qreal zoomPercent);
    ImageZoomChangeSet setFitMode(ImageZoomMode zoomMode);
    ImageZoomChangeSet resetZoom();
    ImageZoomChangeSet updateRenderContext();
    ImageZoomChangeSet setMode(ImagePresentationMode mode);
    void setSecondaryPageVisible(bool visible);
    ImageZoomChangeSet commitPrimaryPageSlot(const ImagePresentationPageSlotSnapshot &slot,
        const ImagePresentationScopeKey &scopeKey, ImagePresentationPrimaryChangePolicy policy);
    ImageZoomChangeSet commitPrimaryPageSlot(
        const ImagePresentationPageSlotSnapshot &slot, const ImagePresentationScopeKey &scopeKey);
    ImageZoomChangeSet commitSecondaryPageSlot(const ImagePresentationPageSlotSnapshot &slot);
    void clearPrimaryPageSlot();
    void clearSecondaryPageSlot();

    ImagePresentationRotationChange resetRotationChange();
    ImagePresentationRotationChange rotateClockwiseChange();
    ImagePresentationRotationChange rotateCounterclockwiseChange();
    bool resetRotation();
    bool rotateClockwise();
    bool rotateCounterclockwise();

    ImageDocumentRenderContext renderContext() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    ImagePresentationRenderProjection renderProjection(DisplayedPageRole role) const;
    ImageDisplaySourceProjection displaySourceProjection(DisplayedPageRole role) const;

private:
    using ZoomStateMutation = ImageZoomWorkflowState::ZoomStateMutation;

    ImagePresentationSnapshot currentSnapshot() const;
    QSize sourceImageSize() const;
    QSize sourceImageSize(const ImagePresentationSnapshot &snapshot) const;
    QSize logicalSinglePageImageSize() const;
    QSize logicalSinglePageImageSize(const ImagePresentationSnapshot &snapshot) const;
    QSize spreadImageSize(const ImagePresentationSnapshot &snapshot) const;
    QSizeF displaySize(const ImagePresentationSnapshot &snapshot) const;
    QSizeF primaryDisplaySize(const ImagePresentationSnapshot &snapshot) const;
    QSizeF secondaryDisplaySize(const ImagePresentationSnapshot &snapshot) const;
    QRectF primaryPageRect(const ImagePresentationSnapshot &snapshot) const;
    QRectF secondaryPageRect(const ImagePresentationSnapshot &snapshot) const;
    ImageZoomChangeSet mutateZoomState(
        const ZoomStateMutation &mutation, bool forceDisplayProjectionUpdate = false);
    void refreshViewportFrame(ImageViewportObservationOrigin origin);
    ImagePresentationRenderProjection renderProjection(
        const ImagePresentationSnapshot &snapshot, DisplayedPageRole role) const;
    ImageDisplaySourceProjection displaySourceProjection(
        const ImagePresentationSnapshot &snapshot, DisplayedPageRole role) const;
    void restoreSnapshot(const ImagePresentationSnapshot &snapshot);

    ImageZoomWorkflowState m_zoomWorkflowState;
    ImageViewportCommandState m_viewportCommands;
    ImagePresentationScopeKey m_scopeKey;
    ImagePresentationPageSlotSnapshot m_primarySlot;
    ImagePresentationPageSlotSnapshot m_secondarySlot;
    bool m_secondaryPageVisible = false;
    bool m_twoPageModeEnabled = false;
    bool m_rightToLeftReadingEnabled = false;
    ImagePresentationMode m_mode = ImagePresentationMode::SinglePage;
    ImagePresentationTransitionState m_transitionState
        = ImagePresentationTransitionState::CommittedActive;
    ImagePresentationSnapshot m_committedSnapshot;
    int m_rotationDegrees = 0;
};
}

#endif
