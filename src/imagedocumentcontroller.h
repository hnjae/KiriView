// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTCONTROLLER_H

#include "filedeletion.h"
#include "imageasyncdependencies.h"
#include "imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"

#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>

namespace KiriView {
struct ImageDocumentRuntime;

class ImageDocumentController final : public QObject
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using FileDeletionFailedCallback = std::function<void(const QString &)>;

    ImageDocumentController(QObject *parent, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback);
    ImageDocumentController(QObject *parent, RenderContextProvider renderContextProvider,
        ChangeCallback changeCallback, ImageAsyncDependencies dependencies,
        FileDeletionFailedCallback fileDeletionFailedCallback = {});
    ~ImageDocumentController() override;

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
    std::unique_ptr<ImageDocumentRuntime> m_runtime;
};
}

#endif
