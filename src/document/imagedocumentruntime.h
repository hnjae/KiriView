// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIME_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIME_H

#include "imagedocumentruntimedependencies.h"
#include "imagedocumentstate.h"
#include "imagedocumenttypes.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecode/predecodedimage.h"
#include "presentation/imagezoomstate.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"
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

namespace KiriView {
class ImageDocumentRuntimeControllers;
struct ImageDocumentSourceLoadRequest;

class ImageDocumentRuntime final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(const std::vector<ImageDocumentChange> &)>;
    using FileDeletionFailedCallback = std::function<void(const QString &)>;
    using UnsupportedOpenedCollectionVideoEnteredCallback = std::function<void(const QString &)>;
    using ContainerNavigationBoundaryReachedCallback = std::function<void(const QString &)>;

    ImageDocumentRuntime(QObject *documentObject, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageDocumentRuntimeDependencyOverrides dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback = {},
        UnsupportedOpenedCollectionVideoEnteredCallback
            unsupportedOpenedCollectionVideoEnteredCallback
        = {},
        ContainerNavigationBoundaryReachedCallback containerNavigationBoundaryReachedCallback = {});
    ~ImageDocumentRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    ImageDocumentStatus status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    OpenedCollectionScopeLocation displayedOpenedCollectionScope() const;
    QSize imageSize() const;
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QPointF viewportContentPosition() const;
    void setViewportContentPosition(const QPointF &viewportContentPosition);
    QSizeF viewportContentSize() const;
    QRectF viewportImageRect() const;
    bool viewportHorizontallyPannable() const;
    bool viewportVerticallyPannable() const;
    bool viewportPannable() const;
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    bool zoomPercentKnown() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
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
    bool unsupportedOpenedCollectionVideo() const;
    std::optional<DisplayedPredecodeImage> primaryDisplayedPredecodeImage() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    DisplayedImageRenderSnapshot renderSnapshot(
        DisplayedPageRole role = DisplayedPageRole::Primary) const;

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
    void loadSource(const ImageDocumentSourceLoadRequest &request);
    void publishChanges(const std::vector<ImageDocumentChange> &changes);

    ImageDocumentChangeBatcher changeBatcher;
    ImageDocumentState state;
    ChangeCallback changeCallback;
    RenderContextProvider renderContextProvider;
    std::unique_ptr<ImageDocumentRuntimeControllers> controllers;
};
}

#endif
