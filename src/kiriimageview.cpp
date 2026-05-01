// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "kiriview/src/avifcompat.cxx.h"

#include <KCoreDirLister>
#include <KFileItem>
#include <KIO/Job>
#include <KIO/StoredTransferJob>
#include <KJob>
#include <QBuffer>
#include <QByteArray>
#include <QCollator>
#include <QFile>
#include <QIODevice>
#include <QImageReader>
#include <QLocale>
#include <QPainter>
#include <QRectF>
#include <QStringList>
#include <Qt>
#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <sys/types.h>
#include <sys/xattr.h>
#include <utility>

namespace {
constexpr int defaultAnimationFrameDelay = 100;
constexpr int minimumAnimationFrameDelay = 10;
constexpr const char *documentPortalHostPathAttribute = "user.document-portal.host-path";

struct ImageNavigationCandidate {
    QUrl url;
    QString name;
};

int normalizedAnimationFrameDelay(int delay)
{
    if (delay < 0) {
        return defaultAnimationFrameDelay;
    }

    return std::max(delay, minimumAnimationFrameDelay);
}

bool isSupportedImageFileName(const QString &name)
{
    static const QStringList supportedExtensions = {
        QStringLiteral("avif"),
        QStringLiteral("bmp"),
        QStringLiteral("gif"),
        QStringLiteral("jpeg"),
        QStringLiteral("jpg"),
        QStringLiteral("png"),
        QStringLiteral("svg"),
        QStringLiteral("webp"),
    };

    const qsizetype dotIndex = name.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= 0 || dotIndex == name.size() - 1) {
        return false;
    }

    return supportedExtensions.contains(name.mid(dotIndex + 1).toCaseFolded());
}

std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !isSupportedImageFileName(name)) {
            continue;
        }

        candidates.push_back(ImageNavigationCandidate { item.url(), name });
    }

    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates.begin(), candidates.end(),
        [&collator](const ImageNavigationCandidate &left, const ImageNavigationCandidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    return candidates;
}

std::optional<QUrl> documentPortalHostUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QString localPath = url.toLocalFile();
    const QByteArray encodedLocalPath = QFile::encodeName(localPath);
    if (encodedLocalPath.isEmpty()) {
        return std::nullopt;
    }

    // File dialogs can return document-portal URLs; navigation needs the real directory.
    const ssize_t valueSize
        = getxattr(encodedLocalPath.constData(), documentPortalHostPathAttribute, nullptr, 0);
    if (valueSize <= 0) {
        return std::nullopt;
    }

    QByteArray value;
    value.resize(valueSize);

    const ssize_t bytesRead = getxattr(encodedLocalPath.constData(),
        documentPortalHostPathAttribute, value.data(), static_cast<std::size_t>(value.size()));
    if (bytesRead <= 0) {
        return std::nullopt;
    }

    value.resize(bytesRead);
    if (value.endsWith('\0')) {
        value.chop(1);
    }

    const QString hostPath = QFile::decodeName(value);
    if (hostPath.isEmpty() || hostPath == localPath) {
        return std::nullopt;
    }

    return QUrl::fromLocalFile(hostPath);
}

QUrl navigationSourceUrl(const QUrl &url)
{
    const std::optional<QUrl> hostUrl = documentPortalHostUrl(url);
    if (hostUrl.has_value()) {
        return hostUrl.value();
    }

    return url;
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
    cancelNavigation();
    cancelLoad();
}

QUrl KiriImageView::sourceUrl() const { return m_sourceUrl; }

void KiriImageView::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    cancelNavigation();
    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
    startLoad();
}

KiriImageView::Status KiriImageView::status() const { return m_status; }

QString KiriImageView::errorString() const { return m_errorString; }

QSize KiriImageView::imageSize() const { return m_imageSize; }

void KiriImageView::openPreviousImage() { openAdjacentImage(NavigationDirection::Previous); }

void KiriImageView::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

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

void KiriImageView::openAdjacentImage(NavigationDirection direction)
{
    if (m_sourceUrl.isEmpty()) {
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(m_sourceUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);

    const quint64 generation = ++m_navigationGeneration;
    m_navigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, generation, direction, currentUrl]() {
            finishNavigation(lister, generation, direction, currentUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, generation](KIO::Job *) { finishNavigationWithError(lister, generation); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishNavigationWithError(lister, generation);
    }
}

void KiriImageView::cancelNavigation()
{
    if (m_navigationLister == nullptr) {
        return;
    }

    auto *lister = m_navigationLister;
    m_navigationLister = nullptr;
    ++m_navigationGeneration;
    lister->stop();
    lister->deleteLater();
}

void KiriImageView::finishNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentUrl)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates
        = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    m_navigationLister = nullptr;
    lister->deleteLater();

    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const ImageNavigationCandidate &candidate) {
            return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return;
    }

    QUrl targetUrl;
    if (direction == NavigationDirection::Previous) {
        if (current == candidates.cbegin()) {
            return;
        }
        targetUrl = std::prev(current)->url;
    } else {
        const auto next = std::next(current);
        if (next == candidates.cend()) {
            return;
        }
        targetUrl = next->url;
    }

    setSourceUrl(targetUrl);
}

void KiriImageView::finishNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    m_navigationLister = nullptr;
    lister->deleteLater();
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

    const QByteArray imageData = KiriView::avifDataWithCompatibilityFixes(data);

    QBuffer buffer;
    buffer.setData(imageData);

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
        startAnimation(imageData, format, loopCount, firstFrameDelay);
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
