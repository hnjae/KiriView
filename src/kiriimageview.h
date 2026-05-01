// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "asyncobjectslot.h"

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
#include <vector>

class KCoreDirLister;

namespace KiriView {
class ImageAnimationPlayer;
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
    struct LoadSession {
        quint64 id = 0;
        QUrl requestedSourceUrl;
        QUrl imageUrl;
        QUrl comicBookRootUrl;
        QUrl containerNavigationUrl;
    };

    class PredecodeCoordinator;

    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl);
    void startLoad();
    void startImageLoad(LoadSession session);
    void startImageDecode(QByteArray data, LoadSession session);
    void startComicBookLoad(LoadSession session);
    void cancelLoad();
    bool isCurrentLoadSession(const LoadSession &session) const;
    void clearLoadSession(const LoadSession &session);
    void setSourceUrlFromResolvedLoad(const QUrl &sourceUrl);
    void openAdjacentImage(KiriView::NavigationDirection direction);
    void openAdjacentComicBookImage(KiriView::NavigationDirection direction);
    void cancelNavigation();
    void finishNavigation(KCoreDirLister *lister, quint64 generation,
        KiriView::NavigationDirection direction, const QUrl &currentUrl);
    void finishNavigationWithError(KCoreDirLister *lister, quint64 generation);
    void openAdjacentContainer(KiriView::NavigationDirection direction);
    void cancelContainerNavigation();
    void finishContainerNavigation(KCoreDirLister *lister, quint64 generation,
        KiriView::NavigationDirection direction, const QUrl &currentContainerUrl);
    void finishContainerNavigationWithError(KCoreDirLister *lister, quint64 generation);
    void openDirectoryContainer(const QUrl &containerUrl);
    void finishDirectoryContainerNavigation(
        KCoreDirLister *lister, quint64 generation, const QUrl &containerUrl);
    void finishDirectoryContainerNavigationWithError(KCoreDirLister *lister, quint64 generation,
        const QUrl &containerUrl, const QString &errorString);
    void openComicBookContainer(const QUrl &containerUrl);
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void updateContainerNavigationFromDisplayedImage();
    void updatePageNavigation();
    void updateFilePageNavigation(const QUrl &currentUrl);
    void updateComicBookPageNavigation(const QUrl &currentUrl, const QUrl &archiveRootUrl);
    void cancelPageNavigationUpdate();
    void finishPageNavigationUpdateWithError(KCoreDirLister *lister, quint64 generation);
    void setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl);
    void setFallbackPageNavigationUrl(const QUrl &currentUrl);
    void clearPageNavigation();
    void scheduleAdjacentImagePredecode();
    void cancelPredecode();
    bool tryDisplayPredecodedImage(LoadSession session);
    void finishLoadWithError(const LoadSession &session, const QString &errorString);
    void finishLoadSuccessfully(
        const LoadSession &session, const QImage &image, bool predecodeCacheable);
    void finishSvgLoadSuccessfully(
        LoadSession session, QByteArray data, const QSize &intrinsicSize);
    void prepareSuccessfulImageLoad(const LoadSession &session);
    void finishSuccessfulImageLoad(const LoadSession &session);
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
    void setDisplaySize(const QSizeF &displaySize);
    void setZoomMode(ZoomMode zoomMode);
    void updateZoomState();
    qreal displayDevicePixelRatio() const;
    int maximumTextureSize() const;
    qreal fitZoomPercent(ZoomMode zoomMode) const;
    qreal fitZoomPercentForImageSize(ZoomMode zoomMode, const QSize &imageSize) const;
    QSizeF displaySizeForZoomPercent(qreal zoomPercent) const;
    QSizeF displaySizeForZoomPercent(qreal zoomPercent, const QSize &imageSize) const;
    void clearImage();

    QUrl m_sourceUrl;
    QUrl m_displayedUrl;
    QUrl m_displayedComicBookRootUrl;
    Status m_status = Status::Null;
    bool m_loading = false;
    QString m_errorString;
    QString m_windowTitleFileName;
    QSize m_imageSize;
    QSizeF m_viewportSize;
    QSizeF m_displaySize;
    qreal m_zoomPercent = 100.0;
    ZoomMode m_zoomMode = ZoomMode::Fit;
    QUrl m_zoomContainerUrl;
    QImage m_image;
    bool m_displayedImageIsSvg = false;
    bool m_displayedImageIsPredecodeCacheable = false;
    QByteArray m_svgData;
    QSize m_svgIntrinsicSize;
    QSize m_svgRasterSize;
    quint64 m_imageRevision = 0;
    std::unique_ptr<KiriView::ImageAnimationPlayer> m_animationPlayer;
    KiriView::AsyncObjectSlot m_imageLoadSlot;
    KiriView::AsyncObjectSlot m_archiveListSlot;
    KiriView::AsyncObjectSlot m_navigationListerSlot;
    KiriView::AsyncObjectSlot m_navigationListSlot;
    KiriView::AsyncObjectSlot m_containerNavigationListerSlot;
    KiriView::AsyncObjectSlot m_containerNavigationListSlot;
    KiriView::AsyncObjectSlot m_pageNavigationListerSlot;
    KiriView::AsyncObjectSlot m_pageNavigationListSlot;
    std::vector<QUrl> m_pageNavigationUrls;
    std::unique_ptr<PredecodeCoordinator> m_predecodeCoordinator;
    int m_currentPageIndex = -1;
    quint64 m_nextLoadSessionId = 0;
    std::optional<LoadSession> m_loadSession;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};

#endif
