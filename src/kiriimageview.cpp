// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include <KIO/StoredTransferJob>
#include <KJob>
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QImageReader>
#include <QPainter>
#include <QRectF>
#include <Qt>
#include <algorithm>
#include <memory>
#include <utility>

namespace {
constexpr int defaultAnimationFrameDelay = 100;
constexpr int minimumAnimationFrameDelay = 10;

int normalizedAnimationFrameDelay(int delay)
{
    if (delay < 0) {
        return defaultAnimationFrameDelay;
    }

    return std::max(delay, minimumAnimationFrameDelay);
}
}

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    m_animationTimer.setSingleShot(true);
    connect(&m_animationTimer, &QTimer::timeout, this, &KiriImageView::advanceAnimationFrame);
}

KiriImageView::~KiriImageView()
{
    stopAnimation();
    cancelLoad();
}

QUrl KiriImageView::sourceUrl() const { return m_sourceUrl; }

void KiriImageView::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
    startLoad();
}

KiriImageView::Status KiriImageView::status() const { return m_status; }

QString KiriImageView::errorString() const { return m_errorString; }

QSize KiriImageView::imageSize() const { return m_imageSize; }

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
        1.0, std::min(bounds.width() / imageSize.width(), bounds.height() / imageSize.height()));
    const QSizeF targetSize(imageSize.width() * scale, imageSize.height() * scale);
    const QRectF targetRect(bounds.center().x() - targetSize.width() / 2.0,
        bounds.center().y() - targetSize.height() / 2.0, targetSize.width(), targetSize.height());

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

    KiriView::ApngDecodeResult apngResult = KiriView::decodeApngAnimation(data);
    if (apngResult.status == KiriView::ApngDecodeStatus::Success) {
        setDisplayedImage(apngResult.animation.frames.front().image);
        setErrorString(QString());
        setStatus(Status::Ready);
        startDecodedAnimation(
            std::move(apngResult.animation.frames), apngResult.animation.loopCount);
        return;
    }
    if (apngResult.status == KiriView::ApngDecodeStatus::Error) {
        clearImage();
        setErrorString(apngResult.errorString);
        setStatus(Status::Error);
        return;
    }

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
    const int firstFrameDelay = reader.nextImageDelay();
    const int loopCount = reader.loopCount();
    const bool hasMoreFrames = reader.canRead();

    setDisplayedImage(image);
    setErrorString(QString());
    setStatus(Status::Ready);

    if (supportsAnimation && hasMoreFrames) {
        startAnimation(data, format, loopCount, firstFrameDelay);
    }
}

void KiriImageView::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_animationData = data;
    m_animationFormat = format;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    QString errorString;
    if (!resetAnimationReader(&errorString)) {
        finishWithAnimationError(errorString);
        return;
    }

    const QImage firstFrame = m_animationReader->read();
    if (firstFrame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    if (m_animationReader->canRead()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(firstFrameDelay));
    }
}

void KiriImageView::startDecodedAnimation(
    std::vector<KiriView::AnimationFrame> frames, int loopCount)
{
    m_decodedAnimationFrames = std::move(frames);
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    if (m_decodedAnimationFrames.size() > 1) {
        m_animationTimer.start(
            normalizedAnimationFrameDelay(m_decodedAnimationFrames.front().delay));
    }
}

void KiriImageView::advanceAnimationFrame()
{
    if (!m_decodedAnimationFrames.empty()) {
        advanceDecodedAnimationFrame();
        return;
    }

    if (m_animationReader == nullptr) {
        return;
    }

    if (!m_animationReader->canRead()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;

        QString errorString;
        if (!resetAnimationReader(&errorString)) {
            finishWithAnimationError(errorString);
            return;
        }
    }

    QImage frame = m_animationReader->read();
    if (frame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    const int delay = normalizedAnimationFrameDelay(m_animationReader->nextImageDelay());
    setDisplayedImage(frame);

    if (m_animationReader->canRead() || hasRemainingAnimationLoops()) {
        m_animationTimer.start(delay);
    } else {
        stopAnimation();
    }
}

void KiriImageView::advanceDecodedAnimationFrame()
{
    if (m_decodedAnimationFrames.empty()) {
        return;
    }

    if (m_decodedAnimationFrameIndex + 1 >= m_decodedAnimationFrames.size()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;
        m_decodedAnimationFrameIndex = 0;
    } else {
        ++m_decodedAnimationFrameIndex;
    }

    const KiriView::AnimationFrame &frame
        = m_decodedAnimationFrames.at(m_decodedAnimationFrameIndex);
    setDisplayedImage(frame.image);

    if (m_decodedAnimationFrameIndex + 1 < m_decodedAnimationFrames.size()
        || hasRemainingAnimationLoops()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(frame.delay));
    } else {
        stopAnimation();
    }
}

bool KiriImageView::resetAnimationReader(QString *errorString)
{
    m_animationReader.reset();
    m_animationBuffer.reset();

    auto buffer = std::make_unique<QBuffer>();
    buffer->setData(m_animationData);

    if (!buffer->open(QIODevice::ReadOnly)) {
        *errorString = tr("Could not read the selected image data.");
        return false;
    }

    auto reader = std::make_unique<QImageReader>(buffer.get(), m_animationFormat);
    reader->setAutoTransform(true);

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    m_animationBuffer = std::move(buffer);
    m_animationReader = std::move(reader);
    return true;
}

bool KiriImageView::hasRemainingAnimationLoops() const
{
    return m_animationLoopCount < 0 || m_completedAnimationLoops < m_animationLoopCount;
}

void KiriImageView::stopAnimation()
{
    m_animationTimer.stop();
    m_animationReader.reset();
    m_animationBuffer.reset();
    m_animationData.clear();
    m_animationFormat.clear();
    m_decodedAnimationFrames.clear();
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = 0;
    m_completedAnimationLoops = 0;
}

void KiriImageView::finishWithAnimationError(const QString &errorString)
{
    clearImage();
    const QString message = errorString.isEmpty()
        ? tr("Could not decode the selected image animation.")
        : errorString;
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
