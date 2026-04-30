// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEVIEW_H
#define KIRIVIEW_KIRIIMAGEVIEW_H

#include <QImage>
#include <QQuickPaintedItem>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace KIO
{
class StoredTransferJob;
}

class QBuffer;
class QMovie;

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

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void sourceUrlChanged();
    void statusChanged();
    void errorStringChanged();
    void imageSizeChanged();

private:
    void startLoad();
    void cancelLoad();
    void finishWithImageData(const QByteArray &data);
    void startAnimation(const QByteArray &data, const QByteArray &format);
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
    std::unique_ptr<QBuffer> m_animationBuffer;
    std::unique_ptr<QMovie> m_movie;
    KIO::StoredTransferJob *m_job = nullptr;
    quint64 m_loadGeneration = 0;
};

#endif
