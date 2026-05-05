// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "decodedimageresult.h"
#include "imageloadplan.h"
#include "imageurl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent, const ImageAsyncDependencies &dependencies)
    : QObject(parent)
    , m_decodeJob(this, dependencies.imageDataLoader, dependencies.imageDataDecoder)
    , m_candidateRepository(dependencies.candidateProvider)
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

void ImageLoader::start(
    ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    cancel();
    m_firstDisplayContext = firstDisplayContext;

    ImageLoadPlan plan = imageLoadPlan(m_loadTickets.next(), std::move(request));
    const ImageLoadSession session = std::move(plan.session);
    if (plan.requiresArchiveListing) {
        m_loadSession = session;
        startArchiveLoad(session);
        return;
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

    m_decodeJob.start(ImageDecodeRequest { session.id, session.location.imageUrl(),
        session.location.archiveDocument(), m_firstDisplayContext });
}

void ImageLoader::startArchiveLoad(ImageLoadSession session)
{
    const ImageCandidateListContext candidateContext
        = ImageCandidateListContext::forArchiveDocument(
            session.location.imageUrl(), session.location.archiveDocument());
    m_archiveListJob = m_candidateRepository.loadImages(
        this, candidateContext,
        [this, session](std::vector<ImageNavigationCandidate> candidates) mutable {
            if (!isCurrentLoadSession(session)) {
                return;
            }

            if (candidates.empty()) {
                finishLoadWithError(session, ImageLoadError::EmptyArchive, QString());
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
    m_firstDisplayContext = {};
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

std::optional<ImageLoadSession> ImageLoader::takeCurrentLoadSession(const ImageLoadSession &session)
{
    if (!isCurrentLoadSession(session)) {
        return std::nullopt;
    }

    std::optional<ImageLoadSession> currentSession = std::move(m_loadSession);
    m_loadSession.reset();
    return currentSession;
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
    finishPredecodedImage(session, std::move(*predecoded));
    return true;
}

void ImageLoader::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    std::optional<ImageLoadSession> currentSession = takeCurrentLoadSession(session);
    if (!currentSession.has_value()) {
        return;
    }

    if (m_error) {
        m_error(*currentSession, error, errorString);
    }
}

void ImageLoader::finishDecodedImage(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    std::optional<ImageLoadSession> currentSession = takeCurrentLoadSession(session);
    if (!currentSession.has_value()) {
        return;
    }

    if (m_decodedImage) {
        m_decodedImage(std::move(*currentSession), std::move(result));
    }
}

void ImageLoader::finishPredecodedImage(ImageLoadSession session, PredecodedImage image)
{
    std::optional<ImageLoadSession> currentSession = takeCurrentLoadSession(session);
    if (!currentSession.has_value()) {
        return;
    }

    if (m_predecodedImage) {
        m_predecodedImage(std::move(*currentSession), std::move(image));
    }
}
}
