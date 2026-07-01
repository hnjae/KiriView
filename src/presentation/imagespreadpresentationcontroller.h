// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H
#define KIRIVIEW_IMAGESPREADPRESENTATIONCONTROLLER_H

#include "cache/imagecachepolicy.h"
#include "decoding/imagedecodedependencies.h"
#include "document/imagedocumentstate.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagepresentationruntime.h"
#include "presentation/imagepresentationstate.h"
#include "presentation/imagespreadgeometry.h"
#include "presentation/imagespreadmodepolicy.h"
#include "presentation/imagespreadnavigation.h"
#include "presentation/imagespreadsecondarypagerefresh.h"
#include "presentation/imagezoomstate.h"
#include "rendering/imagerendercontext.h"

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

namespace kiriview {
class ImagePageSurfaceController;
class ImagePresentationRuntime;
class ImageSecondaryPageController;
enum class ImageSecondaryPageLoadResult;

class ImageSpreadPresentationController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FindPredecodedImageCallback = std::function<std::optional<PredecodedImage>(const QUrl&)>;
    using PageNavigationSnapshotProvider = std::function<ImageDocumentPageNavigationSnapshot()>;
    using ScheduleAdjacentPredecodeCallback = std::function<void()>;

    struct Callbacks
    {
        ChangeCallback change;
        FindPredecodedImageCallback findPredecodedImage;
        PageNavigationSnapshotProvider pageNavigationSnapshot;
        ScheduleAdjacentPredecodeCallback scheduleAdjacentPredecode;
    };

    ImageSpreadPresentationController(QObject* parent, RenderContextProvider renderContextProvider,
        ImageDocumentState& state, ImagePageSurfaceController& primaryPageSurface,
        ImagePresentationRuntime& presentationRuntime, Callbacks callbacks,
        ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, ImageCacheBudgets cacheBudgets);
    ~ImageSpreadPresentationController();

    bool transitionInProgress() const;
    ImagePresentationTransitionState presentationTransitionState() const;
    ImageDocumentStatus status(ImageDocumentStatus documentStatus) const;
    bool loading(bool documentLoading) const;
    bool sameScopeImageNavigationPresentationPending() const;
    const DisplayedImageLocation& committedPrimaryDisplayedImageLocation() const;
    QSize committedImageSize() const;
    QSize committedPrimaryImageSize() const;

    QSize imageSize() const;
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    QPointF viewportContentPosition() const;
    ImageViewportCommand requestViewportContentPosition(QPointF viewportContentPosition);
    bool beginViewportCommandApplication(quint64 commandRevision);
    bool completeViewportCommandApplication(quint64 commandRevision, QPointF actualContentPosition);
    bool acknowledgeViewportCommand(quint64 commandRevision, QPointF actualContentPosition);
    bool observeViewportContentPosition(
        QPointF contentPosition, ImageViewportObservationOrigin origin);
    quint64 viewportCommandRevision() const;
    quint64 viewportAppliedCommandRevision() const;
    quint64 viewportObservationRevision() const;
    ImageViewportCommandStatus viewportCommandStatus() const;
    ImageViewportObservationOrigin viewportObservationOrigin() const;
    QSizeF viewportContentSize() const;
    QRectF viewportImageRect() const;
    bool viewportHorizontallyPannable() const;
    bool viewportVerticallyPannable() const;
    bool viewportPannable() const;
    QRectF visibleItemRect() const;
    qreal zoomPercent() const;
    void requestManualZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    ImageZoomMode fitModeSelection() const;
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
    ImageDisplaySourceProjection displaySourceProjection(DisplayedPageRole role) const;
    void acknowledgeDisplayImageLoad(DisplayedPageRole role, const QUrl& providerUrl,
        quint64 revision, const QString& sourceIdentity, ImageDisplayLoadOutcome outcome);
    void acknowledgeStillImageDisplayLoad(DisplayedPageRole role, const QUrl& providerUrl,
        quint64 revision, const QString& sourceIdentity, ImageDisplayLoadOutcome outcome);

    void commitPrimaryPageSlot(const DisplayedImageLocation& location);
    void clearPrimaryPageSlot();
    void setViewportSize(QSizeF viewportSize);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void rotateClockwise();
    void rotateCounterclockwise();
    void updateRenderContext();
    void refreshSecondaryPage();
    void handleDocumentChange(ImageDocumentChange change);
    bool shouldBeginTransition(int targetPageNumber) const;
    void beginTransition();
    void showTransitionPlaceholder();
    void finishTransition();
    void abortTransition();
    void beginSameScopeImageNavigationPresentation();
    void clearSecondaryPage();
    void shutdown();

    void resetRightToLeftReading();
    void notifyRightToLeftReadingChanged();

private:
    void startSecondaryPageLoad(const QUrl& url);
    void handleSecondaryPageLoadFinished(ImageSecondaryPageLoadResult result,
        const DisplayedImageLocation& location, QSize imageSize);
    void discardSecondaryPage();
    void finishSecondaryPageAsPrimaryOnly();
    void finishSecondaryPageVisible();
    void notifyTransitionChanged();
    QSize spreadImageSize() const;
    bool primaryPageIsWide() const;
    ImageSpreadReadingAvailability readingAvailability() const;
    bool secondaryPageVisibleForNavigation() const;
    ImageSpreadPageNavigationContext pageNavigationContext() const;
    void scheduleAdjacentPredecode();
    ImageDocumentPageNavigationSnapshot pageNavigationSnapshot() const;
    void notifyTwoPageModeChanged();
    void applyActivePresentationChanges(
        ImageZoomChangeSet changes, bool notifyPublicChanges = true);
    void notifyActivePresentationZoomChanged(ImageZoomChangeSet changes);
    void refreshCommittedPresentationFacts(const DisplayedImageLocation& location);
    void clearCommittedPresentationFacts();
    bool updatePresentationPageSlot(DisplayedPageRole role);
    void updateDisplayProjections();
    const ImageViewportFrame& viewportFrame() const;
    void notifyChanges(const std::vector<ImageDocumentChange>& changes);
    void notify(ImageDocumentChange change);

    ImageDocumentState& m_state;
    ImagePageSurfaceController& m_primaryPageSurface;
    ImagePresentationRuntime& m_presentationRuntime;
    Callbacks m_callbacks;
    std::unique_ptr<ImageSecondaryPageController> m_secondaryPageController;
    ImageSpreadSecondaryPageRefresh m_secondaryPageRefresh;
    DisplayedImageLocation m_committedPrimaryDisplayedImageLocation;
    QSize m_committedImageSize;
    QSize m_committedPrimaryImageSize;
    bool m_sameScopeImageNavigationPresentationPending = false;
};
}

#endif
