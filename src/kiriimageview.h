// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "imageloader.h"
#include "imagezoomstate.h"

#include <QByteArray>
#include <QImage>
#include <QQuickItem>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <optional>

namespace KiriView {
class ImageAnimationPlayer;
class ImageNavigationService;
class ImagePredecodeCoordinator;
enum class NavigationDirection : int;
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
    Q_PROPERTY(qreal zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)
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
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ZoomMode zoomMode() const;
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
    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl);
    void startLoad();
    void cancelLoad();
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
    void openAdjacentImage(KiriView::NavigationDirection direction);
    void cancelNavigation();
    void openAdjacentContainer(KiriView::NavigationDirection direction);
    void cancelContainerNavigation();
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void updateContainerNavigationFromDisplayedImage();
    void updatePageNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();
    void scheduleAdjacentImagePredecode();
    void cancelPredecode();
    std::optional<KiriView::PredecodedImage> takePredecodedImage(const QUrl &url) const;
    void finishPredecodedImageLoad(KiriView::ImageLoadSession session, const QImage &image);
    void finishDecodedImageLoad(
        KiriView::ImageLoadSession session, std::shared_ptr<KiriView::DecodedImageResult> result);
    void finishLoadWithError(const KiriView::ImageLoadSession &session,
        KiriView::ImageLoadError error, const QString &errorString);
    void finishLoadSuccessfully(
        const KiriView::ImageLoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishSvgLoadSuccessfully(
        KiriView::ImageLoadSession session, QByteArray data, const QSize &intrinsicSize);
    void prepareSuccessfulImageLoad(const KiriView::ImageLoadSession &session);
    void finishSuccessfulImageLoad(const KiriView::ImageLoadSession &session);
    bool hasDisplayedImage() const;
    void stopAnimation();
    void finishWithAnimationError(const QString &errorString);
    void setDisplayedImage(const QImage &image);
    void setDisplayedSvgImage(
        QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize);
    void clearDisplayedSvgImage();
    bool updateDisplayedSvgRaster();
    void setLoading(bool loading);
    void setStatus(Status status);
    void setErrorString(const QString &errorString);
    void setWindowTitleFileName(const QString &fileName);
    void updateWindowTitleFileName();
    void setImageSize(const QSize &imageSize);
    void applyZoomStateChanges(const KiriView::ImageZoomSnapshot &previous);
    void updateZoomState();
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;
    void clearImage();

    QUrl m_sourceUrl;
    QUrl m_displayedUrl;
    QUrl m_displayedComicBookRootUrl;
    Status m_status = Status::Null;
    bool m_loading = false;
    QString m_errorString;
    QString m_windowTitleFileName;
    KiriView::ImageZoomState m_zoomState;
    QImage m_image;
    bool m_displayedImageIsSvg = false;
    bool m_displayedImageIsPredecodeCacheable = false;
    QByteArray m_svgData;
    QSize m_svgIntrinsicSize;
    QSize m_svgRasterSize;
    quint64 m_imageRevision = 0;
    std::unique_ptr<KiriView::ImageAnimationPlayer> m_animationPlayer;
    std::unique_ptr<KiriView::ImageNavigationService> m_navigationService;
    std::unique_ptr<KiriView::ImageLoader> m_imageLoader;
    std::unique_ptr<KiriView::ImagePredecodeCoordinator> m_predecodeCoordinator;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};

#endif
