// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "imagecontainer.h"
#include "imageurl.h"
#include "kiriimagedecoder.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::comicBookArchiveRootUrl;
using KiriView::containingComicBookArchiveRootUrl;
using KiriView::isUrlInsideArchiveRoot;
}

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
    DataLoader dataLoader, DataDecoder dataDecoder)
    : QObject(parent)
    , m_decodeJob(this, std::move(dataLoader), std::move(dataDecoder))
    , m_candidateRepository(std::move(candidateProvider))
{
    m_decodeJob.setDecodedCallback(
        [this](ImageDecodeRequest request, std::shared_ptr<DecodedImageResult> result) {
            std::optional<ImageLoadSession> session = currentLoadSessionForDecodeRequest(request);
            if (!session.has_value()) {
                return;
            }

            if (const auto *failure = std::get_if<DecodedImageFailure>(result.get())) {
                finishLoadWithError(*session, ImageLoadError::Generic, failure->errorString);
                return;
            }

            finishDecodedImage(*session, std::move(result));
        });
    m_decodeJob.setLoadErrorCallback(
        [this](const ImageDecodeRequest &request, const QString &errorString) {
            std::optional<ImageLoadSession> session = currentLoadSessionForDecodeRequest(request);
            if (!session.has_value()) {
                return;
            }

            finishLoadWithError(*session, ImageLoadError::Generic, errorString);
        });
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

void ImageLoader::start(ImageLoadRequest request)
{
    cancel();

    ImageLoadSession session;
    session.id = m_loadTickets.next();
    session.request = std::move(request);
    session.location.setImageUrl(session.request.sourceUrl());

    const std::optional<QUrl> selectedArchiveRootUrl
        = comicBookArchiveRootUrl(session.request.sourceUrl());
    if (selectedArchiveRootUrl.has_value()) {
        session.location.setComicBookRootUrl(selectedArchiveRootUrl.value());
        m_loadSession = session;
        startComicBookLoad(session);
        return;
    }

    if (isUrlInsideArchiveRoot(
            session.request.sourceUrl(), session.request.displayedComicBookRootUrl())) {
        session.location.setComicBookRootUrl(session.request.displayedComicBookRootUrl());
    } else {
        const std::optional<QUrl> containingArchiveRootUrl
            = containingComicBookArchiveRootUrl(session.request.sourceUrl());
        session.location.setComicBookRootUrl(containingArchiveRootUrl.has_value()
                    && isUrlInsideArchiveRoot(
                        session.request.sourceUrl(), containingArchiveRootUrl.value())
                ? containingArchiveRootUrl.value()
                : QUrl());
    }

    m_loadSession = session;
    if (tryDisplayPredecodedImage(session)) {
        return;
    }

    startImageLoad(session);
}

void ImageLoader::startImageLoad(ImageLoadSession session)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    m_decodeJob.start(ImageDecodeRequest { session.id, session.location.imageUrl() });
}

void ImageLoader::startComicBookLoad(ImageLoadSession session)
{
    const ImageCandidateListContext candidateContext {
        session.location.imageUrl(),
        session.location.comicBookRootUrl(),
        session.location.comicBookRootUrl(),
        ImageCandidateContainerType::ComicBookArchive,
    };
    m_archiveListJob = m_candidateRepository.loadImages(
        this, candidateContext,
        [this, session](std::vector<ImageNavigationCandidate> candidates) mutable {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            if (candidates.empty()) {
                finishLoadWithError(session, ImageLoadError::EmptyComicBookArchive, QString());
                return;
            }

            session.location.setImageUrl(candidates.front().url);
            m_loadSession = session;
            if (m_sourceResolved) {
                m_sourceResolved(session.location.imageUrl());
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
    m_loadTickets.invalidate();
    m_loadSession.reset();
    m_decodeJob.cancel();
    m_archiveListJob.cancel();
}

std::optional<ImageLoadSession> ImageLoader::currentLoadSessionForDecodeRequest(
    const ImageDecodeRequest &request) const
{
    if (!m_loadSession.has_value() || m_loadSession->id != request.id
        || !sameNormalizedUrl(m_loadSession->location.imageUrl(), request.imageUrl)) {
        return std::nullopt;
    }

    return *m_loadSession;
}

bool ImageLoader::isCurrentLoadSession(const ImageLoadSession &session) const
{
    return m_loadTickets.accepts(session.id) && m_loadSession.has_value()
        && m_loadSession->id == session.id;
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

    std::optional<PredecodedImage> predecoded = m_takePredecodedImage(session.location.imageUrl());
    if (!predecoded.has_value()) {
        return false;
    }

    session.location = predecoded->location;
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
