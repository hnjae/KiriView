// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "apngdecoder.h"
#include "predecodecache.h"

#include <QByteArray>
#include <QImage>
#include <QQuickItem>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <cstddef>
#include <memory>
#include <vector>

namespace KIO {
class Job;
class ListJob;
class StoredTransferJob;
}

class QBuffer;
class KCoreDirLister;
class QImageReader;

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
    enum class NavigationDirection {
        Previous,
        Next,
    };

    void setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl);
    void startLoad();
    void startImageLoad(const QUrl &url, quint64 generation);
    void startImageDecode(QByteArray data, quint64 generation);
    void startComicBookLoad(const QUrl &archiveRootUrl, quint64 generation);
    void cancelLoad();
    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentComicBookImage(NavigationDirection direction);
    void cancelNavigation();
    void finishNavigation(KCoreDirLister *lister, quint64 generation, NavigationDirection direction,
        const QUrl &currentUrl);
    void finishNavigationWithError(KCoreDirLister *lister, quint64 generation);
    void openAdjacentContainer(NavigationDirection direction);
    void cancelContainerNavigation();
    void finishContainerNavigation(KCoreDirLister *lister, quint64 generation,
        NavigationDirection direction, const QUrl &currentContainerUrl);
    void finishContainerNavigationWithError(KCoreDirLister *lister, quint64 generation);
    void openDirectoryContainer(const QUrl &containerUrl, quint64 generation);
    void finishDirectoryContainerNavigation(
        KCoreDirLister *lister, quint64 generation, const QUrl &containerUrl);
    void finishDirectoryContainerNavigationWithError(KCoreDirLister *lister, quint64 generation,
        const QUrl &containerUrl, const QString &errorString);
    void openComicBookContainer(const QUrl &containerUrl, quint64 generation);
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void updateContainerNavigationFromDisplayedImage();
    void updatePageNavigation();
    void updateFilePageNavigation(quint64 generation, const QUrl &currentUrl);
    void updateComicBookPageNavigation(
        quint64 generation, const QUrl &currentUrl, const QUrl &archiveRootUrl);
    void cancelPageNavigationUpdate();
    void finishPageNavigationUpdateWithError(KCoreDirLister *lister, quint64 generation);
    void setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl);
    void setFallbackPageNavigationUrl(const QUrl &currentUrl);
    void clearPageNavigation();
    void scheduleAdjacentImagePredecode();
    void scheduleFileAdjacentImagePredecode(quint64 generation);
    void scheduleComicBookAdjacentImagePredecode(quint64 generation);
    void startPredecodeImageLoads(
        const std::vector<QUrl> &urls, const QUrl &comicBookRootUrl, quint64 generation);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);
    void startPredecodeImageDecode(
        QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);
    void cancelPredecode();
    bool tryDisplayPredecodedImage(const QUrl &url);
    void finishLoadWithError(const QString &errorString);
    void finishLoadSuccessfully(const QImage &image, bool predecodeCacheable);
    void finishSvgLoadSuccessfully(QByteArray data, const QSize &intrinsicSize);
    void prepareSuccessfulImageLoad();
    void finishSuccessfulImageLoad();
    void startAnimation(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startDecodedAnimation(std::vector<KiriView::AnimationFrame> frames, int loopCount);
    void advanceAnimationFrame();
    void advanceDecodedAnimationFrame();
    bool resetAnimationReader(QString *errorString);
    bool hasRemainingAnimationLoops() const;
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
    QByteArray m_animationData;
    QByteArray m_animationFormat;
    std::vector<KiriView::AnimationFrame> m_decodedAnimationFrames;
    std::unique_ptr<QBuffer> m_animationBuffer;
    std::unique_ptr<QImageReader> m_animationReader;
    QTimer m_animationTimer;
    std::size_t m_decodedAnimationFrameIndex = 0;
    int m_animationLoopCount = 0;
    int m_completedAnimationLoops = 0;
    KIO::StoredTransferJob *m_job = nullptr;
    KIO::ListJob *m_archiveListJob = nullptr;
    KCoreDirLister *m_navigationLister = nullptr;
    KIO::ListJob *m_navigationListJob = nullptr;
    KCoreDirLister *m_containerNavigationLister = nullptr;
    KIO::ListJob *m_containerNavigationListJob = nullptr;
    KCoreDirLister *m_pageNavigationLister = nullptr;
    KIO::ListJob *m_pageNavigationListJob = nullptr;
    KCoreDirLister *m_predecodeLister = nullptr;
    KIO::ListJob *m_predecodeListJob = nullptr;
    std::vector<QUrl> m_pageNavigationUrls;
    KIO::StoredTransferJob *m_predecodeJob = nullptr;
    QUrl m_activePredecodeUrl;
    KiriView::PredecodeCache m_predecodeCache;
    int m_currentPageIndex = -1;
    quint64 m_loadGeneration = 0;
    quint64 m_navigationGeneration = 0;
    quint64 m_containerNavigationGeneration = 0;
    quint64 m_pageNavigationGeneration = 0;
    quint64 m_predecodeGeneration = 0;
    QUrl m_comicBookRootUrl;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};

#endif
