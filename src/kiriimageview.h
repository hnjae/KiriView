// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "apngdecoder.h"

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
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)
    Q_PROPERTY(
        QSizeF viewportSize READ viewportSize WRITE setViewportSize NOTIFY viewportSizeChanged)
    Q_PROPERTY(QSizeF displaySize READ displaySize NOTIFY displaySizeChanged)
    Q_PROPERTY(qreal zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY zoomPercentChanged)
    Q_PROPERTY(ZoomMode zoomMode READ zoomMode NOTIFY zoomModeChanged)

public:
    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    enum class ZoomMode {
        LogicalScaleFit,
        Manual,
    };
    Q_ENUM(ZoomMode)

    explicit KiriImageView(QQuickItem *parent = nullptr);
    ~KiriImageView() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);

    Status status() const;
    QString errorString() const;
    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QSizeF displaySize() const;
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ZoomMode zoomMode() const;

    Q_INVOKABLE void openPreviousImage();
    Q_INVOKABLE void openNextImage();
    Q_INVOKABLE void resetZoom();

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void errorStringChanged();
    void imageSizeChanged();
    void viewportSizeChanged();
    void displaySizeChanged();
    void zoomPercentChanged();
    void zoomModeChanged();

private:
    enum class NavigationDirection {
        Previous,
        Next,
    };

    void startLoad();
    void startImageLoad(const QUrl &url, quint64 generation);
    void startComicBookLoad(const QUrl &archiveRootUrl, quint64 generation);
    void cancelLoad();
    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentComicBookImage(NavigationDirection direction);
    void cancelNavigation();
    void finishNavigation(KCoreDirLister *lister, quint64 generation, NavigationDirection direction,
        const QUrl &currentUrl);
    void finishNavigationWithError(KCoreDirLister *lister, quint64 generation);
    void finishWithImageData(const QByteArray &data);
    void startAnimation(
        const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay);
    void startDecodedAnimation(std::vector<KiriView::AnimationFrame> frames, int loopCount);
    void advanceAnimationFrame();
    void advanceDecodedAnimationFrame();
    bool resetAnimationReader(QString *errorString);
    bool hasRemainingAnimationLoops() const;
    void stopAnimation();
    void finishWithAnimationError(const QString &errorString);
    void setDisplayedImage(const QImage &image);
    void setStatus(Status status);
    void setErrorString(const QString &errorString);
    void setImageSize(const QSize &imageSize);
    void setDisplaySize(const QSizeF &displaySize);
    void setZoomMode(ZoomMode zoomMode);
    void updateZoomState();
    qreal displayDevicePixelRatio() const;
    qreal logicalScaleFitZoomPercent() const;
    QSizeF displaySizeForZoomPercent(qreal zoomPercent) const;
    void clearImage();

    QUrl m_sourceUrl;
    Status m_status = Status::Null;
    QString m_errorString;
    QSize m_imageSize;
    QSizeF m_viewportSize;
    QSizeF m_displaySize;
    qreal m_zoomPercent = 100.0;
    ZoomMode m_zoomMode = ZoomMode::LogicalScaleFit;
    QImage m_image;
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
    quint64 m_loadGeneration = 0;
    quint64 m_navigationGeneration = 0;
    QUrl m_comicBookRootUrl;
};

#endif
