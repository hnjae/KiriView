// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIME_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIME_H

#include "archive/mediaentrysourcebackend.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentstate.h"
#include "imagedocumenttypes.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagedisplaysourceprojection.h"
#include "presentation/imagepresentationstate.h"
#include "presentation/imageviewportcommandstate.h"
#include "presentation/imageviewportinteraction.h"
#include "presentation/imagezoomstate.h"
#include "rendering/imagerendercontext.h"
#include "system/filedeletion.h"

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;

namespace kiriview {
class ImageDocumentRuntimeControllers;
struct ImageDocumentSourceLoadRequest;

class ImageDocumentRuntime final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(const std::vector<ImageDocumentChange>&)>;
    using FileDeletionFailedCallback = std::function<void(const QString&)>;
    using UnsupportedOpenedCollectionVideoEnteredCallback = std::function<void(const QString&)>;
    using ContainerNavigationBoundaryReachedCallback = std::function<void(const QString&)>;

    ImageDocumentRuntime(QObject* documentObject, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageDocumentRuntimeDependencyOverrides dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback = {},
        UnsupportedOpenedCollectionVideoEnteredCallback
            unsupportedOpenedCollectionVideoEnteredCallback
        = {},
        ContainerNavigationBoundaryReachedCallback containerNavigationBoundaryReachedCallback = {});
    ~ImageDocumentRuntime();

    QUrl sourceUrl() const;
    ImageDocumentPageKind sourceKind() const;
    void setSourceUrl(const QUrl& sourceUrl);
    MediaEntrySourceVideoPlaybackDeviceResult loadOpenedCollectionVideoPlaybackDevice(
        const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& videoUrl) const;
    ImageDocumentStatus status() const;
    bool loading() const;
    QString errorString() const;
    const std::optional<ImageLoadFailure>& loadFailure() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    OpenedCollectionScopeLocation displayedOpenedCollectionScope() const;
    QSize imageSize() const;
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(QSizeF viewportSize);
    QPointF viewportContentPosition() const;
    quint64 requestViewportContentPosition(QPointF viewportContentPosition);
    quint64 requestViewportPanBy(QPointF delta);
    quint64 requestViewportPanToInitialScanPosition();
    quint64 requestViewportPanToFinalScanPosition();
    quint64 requestViewportScanForward();
    quint64 requestViewportScanBackward();
    void requestNextDisplayedImageStartToFinalScanPosition();
    quint64 requestDisplayedImageInitialContentPosition();
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
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    bool zoomPercentKnown() const;
    qreal zoomPercent() const;
    void requestManualZoomPercent(qreal zoomPercent);
    bool requestManualZoomPercentAtCenter(qreal zoomPercent);
    bool requestZoomByStep(qreal stepCount, QPointF viewportAnchorPoint);
    bool requestZoomByStepAtCenter(qreal stepCount);
    bool requestActualSizeAtCenter();
    bool requestToggleFitOrActualSize(QPointF viewportPoint);
    ImageZoomMode zoomMode() const;
    ImageZoomMode fitModeSelection() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    int currentPageNumber() const;
    int currentLastPageNumber() const;
    int pageCount() const;
    ImageDocumentPageNavigationSnapshot pageNavigationSnapshot() const;
    ImageDocumentPageActiveNavigationSnapshot activeNavigationSnapshot() const;
    bool containerNavigationAvailable() const;
    bool ordinaryDirectMediaScopeActive() const;
    bool openedCollectionScopeActive() const;
    bool fileDeletionInProgress() const;
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool secondaryPageVisible() const;
    ImagePresentationTransitionState presentationTransitionState() const;
    bool viewportPointInsideImage(QPointF viewportPoint) const;
    QPointF nearestImageViewportPoint(QPointF viewportPoint) const;
    bool unsupportedOpenedCollectionVideo() const;
    std::optional<DisplayedPredecodeImage> primaryDisplayedPredecodeImage() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    const EmbeddedMetadata& embeddedMetadata() const;
    ImageDisplaySourceProjection displaySourceProjection(
        DisplayedPageRole role = DisplayedPageRole::Primary) const;
    void acknowledgeDisplayImageLoad(DisplayedPageRole role, const QUrl& providerUrl,
        quint64 revision, const QString& sourceIdentity, ImageDisplayLoadOutcome outcome);
    void acknowledgeStillImageDisplayLoad(DisplayedPageRole role, const QUrl& providerUrl,
        quint64 revision, const QString& sourceIdentity, ImageDisplayLoadOutcome outcome);

    void notify(ImageDocumentChange change);
    void setRenderContextProvider(RenderContextProvider provider);
    void shutdown();
    void openPreviousPage();
    void openNextPage();
    void openPreviousSinglePage();
    void openNextSinglePage();
    void openImageAtPage(int pageNumber);
    void openPreviousContainer();
    void openNextContainer();
    void deleteDisplayedFile(FileDeletionMode mode);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void rotateClockwise();
    void rotateCounterclockwise();
    void updateRenderContext();

private:
    ImageDocumentRenderContext renderContext() const;
    quint64 requestViewportInteractionContentPosition(QPointF contentPosition);
    bool requestAnchoredManualZoom(qreal zoomPercent, QPointF viewportAnchorPoint);
    ImageViewportInteractionSnapshot viewportInteractionSnapshot() const;
    void updateViewportInteractionForPublishedChanges(
        const std::vector<ImageDocumentChange>& changes);
    void loadSource(const ImageDocumentSourceLoadRequest& request);
    void publishChanges(const std::vector<ImageDocumentChange>& changes);

    ImageDocumentChangeBatcher changeBatcher;
    ImageDocumentState state;
    ChangeCallback changeCallback;
    RenderContextProvider renderContextProvider;
    std::unique_ptr<ImageDocumentRuntimeControllers> controllers;
    ImageViewportInteraction viewportInteraction;
};
}

#endif
