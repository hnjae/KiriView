// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "imagecontainer.h"
#include "imageiojobs.h"
#include "kiriimagedecoder.h"

#include <QByteArray>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::comicBookArchiveRootUrl;
using KiriView::containingComicBookArchiveRootUrl;
using KiriView::DecodedImageResult;
using KiriView::decodeImageData;
using KiriView::isUrlInsideArchiveRoot;
}

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent)
    : QObject(parent)
{
}

void ImageLoader::setSourceResolvedCallback(SourceResolvedCallback callback)
{
    m_sourceResolved = std::move(callback);
}

void ImageLoader::setErrorCallback(ErrorCallback callback) { m_error = std::move(callback); }

void ImageLoader::setDecodedImageCallback(DecodedImageCallback callback)
{
    m_decodedImage = std::move(callback);
}

void ImageLoader::setPredecodedImageCallback(PredecodedImageCallback callback)
{
    m_predecodedImage = std::move(callback);
}

void ImageLoader::setTakePredecodedImageCallback(TakePredecodedImageCallback callback)
{
    m_takePredecodedImage = std::move(callback);
}

void ImageLoader::start(const QUrl &sourceUrl, const QUrl &displayedComicBookRootUrl,
    const QUrl &containerNavigationUrl)
{
    cancel();

    ImageLoadSession session;
    session.id = ++m_nextLoadSessionId;
    session.requestedSourceUrl = sourceUrl;
    session.imageUrl = sourceUrl;
    session.containerNavigationUrl = containerNavigationUrl;

    const std::optional<QUrl> selectedArchiveRootUrl = comicBookArchiveRootUrl(sourceUrl);
    if (selectedArchiveRootUrl.has_value()) {
        session.comicBookRootUrl = selectedArchiveRootUrl.value();
        m_loadSession = session;
        startComicBookLoad(session);
        return;
    }

    if (isUrlInsideArchiveRoot(sourceUrl, displayedComicBookRootUrl)) {
        session.comicBookRootUrl = displayedComicBookRootUrl;
    } else {
        const std::optional<QUrl> containingArchiveRootUrl
            = containingComicBookArchiveRootUrl(sourceUrl);
        session.comicBookRootUrl = containingArchiveRootUrl.has_value()
                && isUrlInsideArchiveRoot(sourceUrl, containingArchiveRootUrl.value())
            ? containingArchiveRootUrl.value()
            : QUrl();
    }

    m_loadSession = session;
    if (tryDisplayPredecodedImage(session)) {
        return;
    }

    startImageLoad(session);
}

void ImageLoader::startImageLoad(ImageLoadSession session)
{
    startStoredImageDataLoad(
        this, &m_imageLoadSlot, session.imageUrl,
        [this, session](QByteArray data) {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            startImageDecode(std::move(data), session);
        },
        [this, session](const QString &errorString) {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            finishLoadWithError(session, ImageLoadError::Generic, errorString);
        });
}

void ImageLoader::startImageDecode(QByteArray data, ImageLoadSession session)
{
    const QPointer<ImageLoader> loader(this);
    QThreadPool::globalInstance()->start(
        QRunnable::create([loader, data = std::move(data), session]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (loader == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                loader.data(),
                [loader, session, result]() mutable {
                    if (loader == nullptr || !loader->isCurrentLoadSession(session)) {
                        return;
                    }

                    if (!result->success) {
                        loader->finishLoadWithError(
                            session, ImageLoadError::Generic, result->errorString);
                        return;
                    }

                    loader->finishDecodedImage(session, std::move(result));
                },
                Qt::QueuedConnection);
        }));
}

void ImageLoader::startComicBookLoad(ImageLoadSession session)
{
    startArchiveImageCandidateList(
        this, &m_archiveListSlot, session.comicBookRootUrl,
        [this, session](std::vector<ImageNavigationCandidate> candidates) mutable {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            if (candidates.empty()) {
                finishLoadWithError(session, ImageLoadError::EmptyComicBookArchive, QString());
                return;
            }

            session.imageUrl = candidates.front().url;
            m_loadSession = session;
            if (m_sourceResolved) {
                m_sourceResolved(session.imageUrl);
            }
            startImageLoad(session);
        },
        [this, session](const QString &errorString) {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            finishLoadWithError(session, ImageLoadError::Generic, errorString);
        });
}

void ImageLoader::cancel()
{
    m_loadSession.reset();
    m_imageLoadSlot.cancel();
    m_archiveListSlot.cancel();
}

bool ImageLoader::isCurrentLoadSession(const ImageLoadSession &session) const
{
    return m_loadSession.has_value() && m_loadSession->id == session.id;
}

void ImageLoader::clearLoadSession(const ImageLoadSession &session)
{
    if (isCurrentLoadSession(session)) {
        m_loadSession.reset();
    }
}

bool ImageLoader::tryDisplayPredecodedImage(ImageLoadSession session)
{
    if (!m_takePredecodedImage) {
        return false;
    }

    std::optional<PredecodedImage> predecoded = m_takePredecodedImage(session.imageUrl);
    if (!predecoded.has_value()) {
        return false;
    }

    session.comicBookRootUrl = predecoded->comicBookRootUrl;
    m_loadSession = session;
    finishPredecodedImage(session, predecoded->image);
    return true;
}

void ImageLoader::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    clearLoadSession(session);
    if (m_error) {
        m_error(session, error, errorString);
    }
}

void ImageLoader::finishDecodedImage(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    clearLoadSession(session);
    if (m_decodedImage) {
        m_decodedImage(std::move(session), std::move(result));
    }
}

void ImageLoader::finishPredecodedImage(ImageLoadSession session, const QImage &image)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    clearLoadSession(session);
    if (m_predecodedImage) {
        m_predecodedImage(std::move(session), image);
    }
}
}
