// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIME_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIME_H

#include "filedeletion.h"
#include "imageasyncdependencies.h"
#include "imagedocumenteffects.h"
#include "imagedocumentstate.h"
#include "imagedocumenttypes.h"
#include "imagenavigationtypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"

#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentEffectExecutor;
struct ImageDocumentEffectOperations;
class ImageDocumentLoadController;
class ImageDocumentPredecodeController;
class ImageOpenController;
class ImageNavigationService;
class ImagePresentationController;
class ImageSpreadPresentationController;

class ImageDocumentRuntime final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FileDeletionFailedCallback = std::function<void(const QString &)>;

    ImageDocumentRuntime(QObject *documentObject, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageAsyncDependencies dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback);
    ~ImageDocumentRuntime();

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);
    ImageDocumentStatus status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    QSize imageSize() const;
    QSize primaryImageSize() const;
    QSize secondaryImageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    int currentPageNumber() const;
    int currentLastPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    bool fileDeletionInProgress() const;
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool secondaryPageVisible() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface(
        DisplayedPageRole role = DisplayedPageRole::Primary) const;
    const QImage &image() const;
    quint64 imageRevision(DisplayedPageRole role = DisplayedPageRole::Primary) const;

    void dispatchEffect(ImageDocumentEffect effect);
    void notify(ImageDocumentChange change);
    void shutdown();
    void openPreviousImage();
    void openNextImage();
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
    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentContainer(NavigationDirection direction);
    void openImageAtRelativePageOffset(int offset);
    ImageDocumentEffectOperations effectOperations();

    ImageDocumentState state;
    ChangeCallback changeCallback;
    std::unique_ptr<ArchiveDocumentSessionStore> archiveSessionStore;
    std::unique_ptr<ImageDocumentDeletionController> documentDeletionController;
    std::unique_ptr<ImagePresentationController> presentationController;
    std::unique_ptr<ImageOpenController> openController;
    std::unique_ptr<ImageNavigationService> navigationService;
    std::unique_ptr<ImageDocumentPredecodeController> predecodeController;
    std::unique_ptr<ImageSpreadPresentationController> spreadController;
    std::unique_ptr<ImageDocumentLoadController> loadController;
    std::unique_ptr<ImageDocumentEffectExecutor> effectExecutor;
};
}

#endif
