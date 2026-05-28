// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H
#define KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H

#include "decoding/imagedecodedependencies.h"
#include "document/imagedocumentstate.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagespreadgeometry.h"
#include "presentation/imagespreadmodepolicy.h"
#include "presentation/imagespreadnavigation.h"
#include "presentation/imagespreadsecondarypagerefresh.h"
#include "presentation/imageviewportframe.h"
#include "presentation/imagezoomstate.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
class ImagePresentationController;
class ImageSecondaryPageController;
class ImageSpreadModeController;
class ImageSpreadZoomController;
enum class ImageSecondaryPageLoadResult;

class ImageSpreadPresentationController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FindPredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl &)>;
    using PageNavigationSnapshotProvider = std::function<ImageDocumentPageNavigationSnapshot()>;
    using ScheduleAdjacentPredecodeCallback = std::function<void()>;

    struct Callbacks {
        ChangeCallback change;
        FindPredecodedImageCallback findPredecodedImage;
        PageNavigationSnapshotProvider pageNavigationSnapshot;
        ScheduleAdjacentPredecodeCallback scheduleAdjacentPredecode;
    };

    ImageSpreadPresentationController(QObject *parent, RenderContextProvider renderContextProvider,
        ImageDocumentState &state, ImagePresentationController &primaryPresentation,
        Callbacks callbacks, ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, qsizetype predecodeCacheByteBudget = 0);
    ~ImageSpreadPresentationController();

    bool transitionInProgress() const;
    ImageDocumentStatus status(ImageDocumentStatus documentStatus) const;
    bool loading(bool documentLoading) const;

    QSize imageSize() const;
    QSize secondaryImageSize() const;
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    QPointF viewportContentPosition() const;
    void setViewportContentPosition(const QPointF &viewportContentPosition);
    QSizeF viewportContentSize() const;
    QRectF viewportImageRect() const;
    bool viewportHorizontallyPannable() const;
    bool viewportVerticallyPannable() const;
    bool viewportPannable() const;
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    int currentLastPageNumber() const;
    ImageDocumentPageActiveNavigationSnapshot activeNavigationSnapshot() const;
    ImageSpreadPageNavigationTarget imageDocumentPageNavigationTarget(
        NavigationDirection direction) const;
    int relativePageNavigationTarget(int offset) const;

    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool twoPageModeActive() const;
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool rightToLeftReadingActive() const;
    bool secondaryPageVisible() const;
    std::optional<DisplayedPredecodeImage> secondaryDisplayedPredecodeImage() const;
    DisplayedImageRenderSnapshot renderSnapshot(DisplayedPageRole role) const;

    void setViewportSize(const QSizeF &viewportSize);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void rotateClockwise();
    void rotateCounterclockwise();
    void updateRenderContext();
    void refreshSecondaryPage();
    void handleDocumentChange(ImageDocumentChange change);
    bool shouldBeginTransition(int targetPageNumber) const;
    void beginTransition();
    void finishTransition();
    void clearSecondaryPage();
    void shutdown();

    void resetRightToLeftReading();
    void notifyRightToLeftReadingChanged();

private:
    void startSecondaryPageLoad(const QUrl &url);
    void handleSecondaryPageLoadFinished(ImageSecondaryPageLoadResult result,
        const DisplayedImageLocation &location, const QSize &imageSize);
    void finishSecondaryPageAsPrimaryOnly();
    void finishSecondaryPageVisible();
    void notifyTransitionChanged();
    ImageZoomChangeSet updateZoomState();
    QSize spreadImageSize() const;
    bool primaryPageIsWide() const;
    ImageSpreadReadingAvailability readingAvailability() const;
    ImageSpreadPageNavigationContext pageNavigationContext() const;
    void scheduleAdjacentPredecode();
    ImageDocumentPageNavigationSnapshot pageNavigationSnapshot() const;
    void notifyTwoPageModeChanged();
    void applySpreadZoomChanges(const ImageZoomChangeSet &changes, bool notifyPublicChanges = true);
    void notifySpreadZoomChanged(const ImageZoomChangeSet &changes);
    bool refreshViewportFrame(bool forceApplyVisibleItemRect = false);
    bool refreshViewportFrameForContentPosition(
        const QPointF &contentPosition, bool forceApplyVisibleItemRect = false);
    void applyViewportFrameVisibleItemRect(bool force = false);
    void notifyChanges(const std::vector<ImageDocumentChange> &changes);
    void notify(ImageDocumentChange change);

    ImageDocumentState &m_state;
    ImagePresentationController &m_primaryPresentation;
    Callbacks m_callbacks;
    std::unique_ptr<ImageSecondaryPageController> m_secondaryPageController;
    std::unique_ptr<ImageSpreadModeController> m_modeController;
    std::unique_ptr<ImageSpreadZoomController> m_zoomController;
    ImageSpreadSecondaryPageRefresh m_secondaryPageRefresh;
    ImageViewportFrame m_viewportFrame;
};
}

#endif
