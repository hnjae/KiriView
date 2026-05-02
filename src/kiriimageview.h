// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "imagedocumenttypes.h"

#include <QPointF>
#include <QQuickItem>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace KiriView {
class ImageDocumentController;
}

class KiriImageView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(
        QString windowTitleFileName READ windowTitleFileName NOTIFY windowTitleFileNameChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
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

    explicit KiriImageView(QQuickItem *parent = nullptr);
    ~KiriImageView() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);

    Status status() const;
    bool loading() const;
    QString errorString() const;
    QString windowTitleFileName() const;
    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
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

    Q_INVOKABLE void openPreviousImage();
    Q_INVOKABLE void openNextImage();
    Q_INVOKABLE void openImageAtPage(int pageNumber);
    Q_INVOKABLE void openPreviousContainer();
    Q_INVOKABLE void openNextContainer();
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void setFitMode(KiriImageView::ZoomMode zoomMode);
    Q_INVOKABLE QPointF panContentPosition(
        const QPointF &contentPosition, const QPointF &delta) const;
    Q_INVOKABLE bool viewportPointInsideImage(
        const QPointF &contentPosition, const QPointF &viewportPoint) const;
    Q_INVOKABLE double clampedManualZoomPercent(double zoomPercent) const;
    Q_INVOKABLE QPointF zoomContentPosition(const QPointF &contentPosition,
        const QPointF &viewportAnchorPoint, double nextZoomPercent) const;

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void loadingChanged();
    void errorStringChanged();
    void windowTitleFileNameChanged();
    void imageSizeChanged();
    void viewportSizeChanged();
    void displaySizeChanged();
    void zoomPercentChanged();
    void zoomModeChanged();
    void pageNavigationChanged();
    void containerNavigationChanged();

private:
    void handleDocumentChange(KiriView::ImageDocumentChange change);
    KiriView::ImageDocumentRenderContext renderContext() const;
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;

    std::unique_ptr<KiriView::ImageDocumentController> m_documentController;
};

#endif
