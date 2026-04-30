// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include "apngdecoder.h"

#include <QByteArray>
#include <QImage>
#include <QQuickPaintedItem>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <cstddef>
#include <memory>
#include <vector>

namespace KIO {
class Job;
class StoredTransferJob;
}

class QBuffer;
class KCoreDirLister;
class QImageReader;

class KiriImageView : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSourceUrl NOTIFY sourceUrlChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageSizeChanged)

public:
    enum class Status {
        Null,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    explicit KiriImageView(QQuickItem *parent = nullptr);
    ~KiriImageView() override;

    QUrl sourceUrl() const;
    void setSourceUrl(const QUrl &sourceUrl);

    Status status() const;
    QString errorString() const;
    QSize imageSize() const;

    Q_INVOKABLE void openPreviousImage();
    Q_INVOKABLE void openNextImage();

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void errorStringChanged();
    void imageSizeChanged();

private:
    enum class NavigationDirection {
        Previous,
        Next,
    };

    void startLoad();
    void cancelLoad();
    void openAdjacentImage(NavigationDirection direction);
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
    void clearImage();

    QUrl m_sourceUrl;
    Status m_status = Status::Null;
    QString m_errorString;
    QSize m_imageSize;
    QImage m_image;
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
    KCoreDirLister *m_navigationLister = nullptr;
    quint64 m_loadGeneration = 0;
    quint64 m_navigationGeneration = 0;
};

#endif
