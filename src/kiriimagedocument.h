// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDOCUMENT_H
#define KIRIVIEW_KIRIIMAGEDOCUMENT_H

#include "imagedocumenttypes.h"
#include "imagesurface.h"

#include <QImage>
#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <functional>
#include <memory>

namespace KiriView {
class ImageDocumentController;
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
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(QRectF visibleItemRect READ visibleItemRect WRITE setVisibleItemRect NOTIFY
            visibleItemRectChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(double zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)
    Q_PROPERTY(int minimumManualZoomPercent READ minimumManualZoomPercent CONSTANT)
    Q_PROPERTY(int maximumManualZoomPercent READ maximumManualZoomPercent CONSTANT)
    Q_PROPERTY(int zoomStepPercent READ zoomStepPercent CONSTANT)
    Q_PROPERTY(QStringList openDialogNameFilters READ openDialogNameFilters CONSTANT)
    Q_PROPERTY(int currentPageNumber READ currentPageNumber NOTIFY pageNavigationChanged)
    Q_PROPERTY(int imageCount READ imageCount NOTIFY pageNavigationChanged)
    Q_PROPERTY(bool containerNavigationAvailable READ containerNavigationAvailable NOTIFY
            containerNavigationChanged)

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

    explicit KiriImageDocument(QObject *parent = nullptr);
    ~KiriImageDocument() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);

    Status status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QUrl displayedUrl() const;
    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    QSizeF displaySize() const;
    double zoomPercent() const;
    void setZoomPercent(double zoomPercent);
    ZoomMode zoomMode() const;
    int minimumManualZoomPercent() const;
    int maximumManualZoomPercent() const;
    int zoomStepPercent() const;
    QStringList openDialogNameFilters() const;
    int currentPageNumber() const;
    int imageCount() const;
    bool containerNavigationAvailable() const;
    std::shared_ptr<KiriView::DisplayedImageSurface> imageSurface() const;
    const QImage &image() const;
    quint64 imageRevision() const;
    quint64 renderRevision() const;

    void setRenderContextProvider(RenderContextProvider provider);

    Q_INVOKABLE void openPreviousImage();
    Q_INVOKABLE void openNextImage();
    Q_INVOKABLE void openImageAtPage(int pageNumber);
    Q_INVOKABLE void openPreviousContainer();
    Q_INVOKABLE void openNextContainer();
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void setFitMode(KiriImageDocument::ZoomMode zoomMode);
    Q_INVOKABLE double clampedManualZoomPercent(double zoomPercent) const;
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
    void visibleItemRectChanged();
    void displaySizeChanged();
    void zoomPercentChanged();
    void zoomModeChanged();
    void pageNavigationChanged();
    void containerNavigationChanged();
    void repaintRequested();

private:
    void handleDocumentChange(KiriView::ImageDocumentChange change);
    KiriView::ImageDocumentRenderContext renderContext() const;

    RenderContextProvider m_renderContextProvider;
    std::unique_ptr<KiriView::ImageDocumentController> m_documentController;
};

#endif
