// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include <KIO/StoredTransferJob>
#include <KJob>
#include <QBuffer>
#include <QByteArray>
#include <QImageReader>
#include <QIODevice>
#include <QMetaObject>
#include <QMovie>
#include <QPainter>
#include <QPointer>
#include <QRectF>
#include <Qt>
#include <algorithm>
#include <memory>
#include <utility>

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

KiriImageView::~KiriImageView()
{
    stopAnimation();
    cancelLoad();
}

QUrl KiriImageView::sourceUrl() const
{
    return m_sourceUrl;
}

void KiriImageView::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
    startLoad();
}

KiriImageView::Status KiriImageView::status() const
{
    return m_status;
}

QString KiriImageView::errorString() const
{
    return m_errorString;
}

QSize KiriImageView::imageSize() const
{
    return m_imageSize;
}

void KiriImageView::paint(QPainter *painter)
{
    if (m_image.isNull()) {
        return;
    }

    const QRectF bounds = boundingRect();
    if (bounds.isEmpty()) {
        return;
    }

    const QSize imageSize = m_image.size();
    const qreal scale = std::min<qreal>(
        1.0,
        std::min(bounds.width() / imageSize.width(), bounds.height() / imageSize.height()));
    const QSizeF targetSize(imageSize.width() * scale, imageSize.height() * scale);
    const QRectF targetRect(bounds.center().x() - targetSize.width() / 2.0,
                            bounds.center().y() - targetSize.height() / 2.0,
                            targetSize.width(),
                            targetSize.height());

    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(targetRect, m_image);
}

void KiriImageView::startLoad()
{
    cancelLoad();
    clearImage();
    setErrorString(QString());

    if (m_sourceUrl.isEmpty()) {
        setStatus(Status::Null);
        return;
    }

    setStatus(Status::Loading);

    const quint64 generation = ++m_loadGeneration;
    auto *job = KIO::storedGet(m_sourceUrl, KIO::NoReload, KIO::HideProgressInfo);
    m_job = job;

    connect(job, &KJob::result, this, [this, job, generation](KJob *finishedJob) {
        if (generation != m_loadGeneration || job != m_job) {
            return;
        }

        m_job = nullptr;

        if (finishedJob->error() != KJob::NoError) {
            clearImage();
            setErrorString(finishedJob->errorString());
            setStatus(Status::Error);
            return;
        }

        finishWithImageData(job->data());
    });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_job) {
            m_job = nullptr;
        }
    });
}

void KiriImageView::cancelLoad()
{
    if (m_job == nullptr) {
        return;
    }

    auto *job = m_job;
    m_job = nullptr;
    ++m_loadGeneration;
    job->kill(KJob::Quietly);
}

void KiriImageView::finishWithImageData(const QByteArray &data)
{
    stopAnimation();

    QBuffer buffer;
    buffer.setData(data);

    if (!buffer.open(QIODevice::ReadOnly)) {
        clearImage();
        setErrorString(tr("Could not read the selected image data."));
        setStatus(Status::Error);
        return;
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const bool supportsAnimation = reader.supportsAnimation();

    QImage image = reader.read();
    if (image.isNull()) {
        clearImage();
        setErrorString(reader.errorString());
        setStatus(Status::Error);
        return;
    }

    const QByteArray format = reader.format();

    setDisplayedImage(image);
    setErrorString(QString());
    setStatus(Status::Ready);

    if (supportsAnimation) {
        startAnimation(data, format);
    }
}

void KiriImageView::startAnimation(const QByteArray &data, const QByteArray &format)
{
    auto buffer = std::make_unique<QBuffer>();
    buffer->setData(data);

    if (!buffer->open(QIODevice::ReadOnly)) {
        finishWithAnimationError(tr("Could not read the selected image data."));
        return;
    }

    auto movie = std::make_unique<QMovie>(buffer.get(), format);
    if (!movie->isValid()) {
        finishWithAnimationError(movie->lastErrorString());
        return;
    }

    auto *moviePtr = movie.get();
    const QPointer<QMovie> movieGuard(moviePtr);

    connect(moviePtr, &QMovie::frameChanged, this, [this, movieGuard]() {
        auto *movie = movieGuard.data();
        if (movie == nullptr || movie != m_movie.get()) {
            return;
        }

        const QImage frame = movie->currentImage();
        if (frame.isNull()) {
            return;
        }

        setDisplayedImage(frame);
    });

    connect(moviePtr, &QMovie::error, this, [this, movieGuard]() {
        auto *movie = movieGuard.data();
        if (movie == nullptr || movie != m_movie.get()) {
            return;
        }

        const QString errorString = movie->lastErrorString();
        movie->stop();
        QMetaObject::invokeMethod(
            this,
            [this, movieGuard, errorString]() {
                auto *movie = movieGuard.data();
                if (movie == nullptr || movie != m_movie.get()) {
                    return;
                }

                finishWithAnimationError(errorString);
            },
            Qt::QueuedConnection);
    });

    m_animationBuffer = std::move(buffer);
    m_movie = std::move(movie);
    m_movie->start();
}

void KiriImageView::stopAnimation()
{
    if (m_movie != nullptr) {
        m_movie->stop();
        m_movie.reset();
    }

    if (m_animationBuffer != nullptr) {
        m_animationBuffer->close();
        m_animationBuffer.reset();
    }
}

void KiriImageView::finishWithAnimationError(const QString &errorString)
{
    clearImage();
    const QString message =
        errorString.isEmpty() ? tr("Could not decode the selected image animation.") : errorString;
    setErrorString(message);
    setStatus(Status::Error);
}

void KiriImageView::setDisplayedImage(const QImage &image)
{
    m_image = image;
    setImageSize(m_image.size());
    update();
}

void KiriImageView::setStatus(Status status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged();
}

void KiriImageView::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    Q_EMIT errorStringChanged();
}

void KiriImageView::setImageSize(const QSize &imageSize)
{
    if (m_imageSize == imageSize) {
        return;
    }

    m_imageSize = imageSize;
    Q_EMIT imageSizeChanged();
}

void KiriImageView::clearImage()
{
    stopAnimation();

    if (!m_image.isNull()) {
        m_image = QImage();
        update();
    }

    setImageSize(QSize());
}
