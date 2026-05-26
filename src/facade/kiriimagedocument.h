// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDOCUMENT_H
#define KIRIVIEW_KIRIIMAGEDOCUMENT_H

#include "document/imagedocumentruntimedependencies.h"
#include "document/imagedocumenttypes.h"
#include "predecode/predecodedimage.h"
#include "rendering/imagesurface.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
class ImageDocumentRuntime;
}

class KiriImageDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(
        QString windowTitleFileName READ windowTitleFileName NOTIFY windowTitleFileNameChanged)
    Q_PROPERTY(QUrl displayedUrl READ displayedUrl NOTIFY displayedUrlChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(QSize primaryImageSize READ primaryImageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(QSize secondaryImageSize READ secondaryImageSize NOTIFY twoPageModeChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(QPointF viewportContentPosition READ viewportContentPosition WRITE
            setViewportContentPosition NOTIFY viewportFrameChanged)
    Q_PROPERTY(QSizeF viewportContentSize READ viewportContentSize NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF viewportImageRect READ viewportImageRect NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportHorizontallyPannable READ viewportHorizontallyPannable NOTIFY
            viewportFrameChanged)
    Q_PROPERTY(
        bool viewportVerticallyPannable READ viewportVerticallyPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(bool viewportPannable READ viewportPannable NOTIFY viewportFrameChanged)
    Q_PROPERTY(QRectF visibleItemRect READ visibleItemRect WRITE setVisibleItemRect NOTIFY
            visibleItemRectChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF primaryDisplaySize READ primaryDisplaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(QSizeF secondaryDisplaySize READ secondaryDisplaySize NOTIFY twoPageModeChanged)
    Q_PROPERTY(bool zoomPercentKnown READ zoomPercentKnown NOTIFY zoomPercentKnownChanged)
    Q_PROPERTY(double zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(int rotationDegrees READ rotationDegrees NOTIFY rotationDegreesChanged)
    Q_PROPERTY(int minimumManualZoomPercent READ minimumManualZoomPercent CONSTANT)
    Q_PROPERTY(int maximumManualZoomPercent READ maximumManualZoomPercent NOTIFY
            maximumManualZoomPercentChanged)
    Q_PROPERTY(double zoomStepFactor READ zoomStepFactor CONSTANT)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(int currentPageNumber READ currentPageNumber NOTIFY pageNavigationChanged)
    Q_PROPERTY(int currentLastPageNumber READ currentLastPageNumber NOTIFY pageNavigationChanged)
    Q_PROPERTY(int imageCount READ imageCount NOTIFY pageNavigationChanged)
    Q_PROPERTY(bool containerNavigationAvailable READ containerNavigationAvailable NOTIFY
            containerNavigationChanged)
    Q_PROPERTY(bool ordinaryDirectMediaScopeActive READ ordinaryDirectMediaScopeActive NOTIFY
            documentScopeChanged)
    Q_PROPERTY(
        bool openedDocumentScopeActive READ openedDocumentScopeActive NOTIFY documentScopeChanged)
    Q_PROPERTY(bool fileDeletionInProgress READ fileDeletionInProgress NOTIFY
            fileDeletionInProgressChanged)
    Q_PROPERTY(bool twoPageModeEnabled READ twoPageModeEnabled WRITE setTwoPageModeEnabled NOTIFY
            twoPageModeChanged)
    Q_PROPERTY(bool twoPageModeAvailable READ twoPageModeAvailable NOTIFY twoPageModeChanged)
    Q_PROPERTY(bool rightToLeftReadingEnabled READ rightToLeftReadingEnabled WRITE
            setRightToLeftReadingEnabled NOTIFY rightToLeftReadingChanged)
    Q_PROPERTY(bool rightToLeftReadingAvailable READ rightToLeftReadingAvailable NOTIFY
            rightToLeftReadingChanged)
    Q_PROPERTY(bool secondaryPageVisible READ secondaryPageVisible NOTIFY twoPageModeChanged)

public:
    using RenderContextProvider = std::function<KiriView::ImageDocumentRenderContext()>;

    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    enum class ZoomMode {
        Fit,
        FitHeight,
        FitWidth,
        Manual,
    };
    Q_ENUM(ZoomMode)

    enum class DeletionMode {
        MoveToTrash,
        DeletePermanently,
    };
    Q_ENUM(DeletionMode)

    explicit KiriImageDocument(QObject *parent = nullptr);
    explicit KiriImageDocument(
        KiriView::ImageDocumentRuntimeDependencyOverrides dependencies, QObject *parent = nullptr);
    ~KiriImageDocument() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);

    Status status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
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
    double zoomPercent() const;
    void setZoomPercent(double zoomPercent);
    ZoomMode zoomMode() const;
    int rotationDegrees() const;
    int minimumManualZoomPercent() const;
    int maximumManualZoomPercent() const;
    double zoomStepFactor() const;
    QStringList openDialogNameFilters() const;
    int currentPageNumber() const;
    int currentLastPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    bool ordinaryDirectMediaScopeActive() const;
    bool openedDocumentScopeActive() const;
    bool fileDeletionInProgress() const;
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool enabled);
    bool twoPageModeAvailable() const;
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool secondaryPageVisible() const;
    std::optional<KiriView::DisplayedPredecodeImage> primaryDisplayedPredecodeImage() const;
    KiriView::ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;
    KiriView::DisplayedImageRenderSnapshot renderSnapshot(
        KiriView::DisplayedPageRole role = KiriView::DisplayedPageRole::Primary) const;

    void setRenderContextProvider(RenderContextProvider provider);

    Q_INVOKABLE void openPreviousImage();
    Q_INVOKABLE void openNextImage();
    Q_INVOKABLE void openPreviousSinglePage();
    Q_INVOKABLE void openNextSinglePage();
    Q_INVOKABLE void openImageAtPage(int pageNumber);
    Q_INVOKABLE void openPreviousContainer();
    Q_INVOKABLE void openNextContainer();
    Q_INVOKABLE void deleteDisplayedFile(KiriImageDocument::DeletionMode mode);
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void setFitMode(KiriImageDocument::ZoomMode zoomMode);
    Q_INVOKABLE void rotateClockwise();
    Q_INVOKABLE void rotateCounterclockwise();
    Q_INVOKABLE double clampedManualZoomPercent(double zoomPercent) const;
    Q_INVOKABLE double steppedManualZoomPercent(double stepCount) const;
    Q_INVOKABLE void updateRenderContext();

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void loadingChanged();
    void errorStringChanged();
    void windowTitleFileNameChanged();
    void displayedUrlChanged();
    void imageSizeChanged();
    void viewportSizeChanged();
    void viewportFrameChanged();
    void visibleItemRectChanged();
    void displaySizeChanged();
    void zoomPercentKnownChanged();
    void zoomPercentChanged();
    void zoomModeChanged();
    void rotationDegreesChanged();
    void maximumManualZoomPercentChanged();
    void pageNavigationChanged();
    void containerNavigationChanged();
    void documentScopeChanged();
    void fileDeletionInProgressChanged();
    void twoPageModeChanged();
    void rightToLeftReadingChanged();
    void fileDeletionFailed(const QString &errorString);
    void repaintRequested();

private:
    void handleDocumentChanges(const std::vector<KiriView::ImageDocumentChange> &changes);

    std::unique_ptr<KiriView::ImageDocumentRuntime> m_runtime;
};

#endif
