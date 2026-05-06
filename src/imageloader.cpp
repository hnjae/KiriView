// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "decodedimageresult.h"
#include "imagecallback.h"
#include "imageloadplan.h"
#include "imageurl.h"

#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent, Callbacks callbacks)
    : ImageLoader(parent, defaultImageAsyncDependencies(), std::move(callbacks))
{
}

ImageLoader::ImageLoader(QObject *parent, const ImageAsyncDependencies &dependencies)
    : ImageLoader(parent, dependencies, {})
{
}

ImageLoader::ImageLoader(
    QObject *parent, const ImageAsyncDependencies &dependencies, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_decodeJob(this, dependencies.imageDecode,
          ImageDecodeJob::Callbacks {
              [this](ImageDecodeRequest request, DecodedImageResult result) {
                  finishDecodeResult(std::move(request), std::move(result));
              },
              [this](const ImageDecodeRequest &request, const QString &errorString) {
                  finishImageLoadError(request, errorString);
              },
          })
    , m_candidateRepository(dependencies.candidateProvider)
{
}

void ImageLoader::finishDecodeResult(ImageDecodeRequest request, DecodedImageResult result)
{
    std::optional<ImageLoadSession> session = currentLoadSessionForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    if (const DecodedImageFailure *failure = result.failure()) {
        finishLoadWithError(*session, ImageLoadError::Generic, failure->errorString);
        return;
    }

    std::optional<DecodedImage> image = std::move(result).takeImage();
    if (!image.has_value()) {
        return;
    }

    finishDecodedImage(std::move(*session), std::move(*image));
}

void ImageLoader::finishImageLoadError(
    const ImageDecodeRequest &request, const QString &errorString)
{
    std::optional<ImageLoadSession> session = currentLoadSessionForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    finishLoadWithError(*session, ImageLoadError::Generic, errorString);
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

    m_decodeJob.start(
        ImageDecodeRequest::fromLocation(session.id, session.location, m_firstDisplayContext));
}

void ImageLoader::startArchiveLoad(ImageLoadSession session)
{
    const ImageCandidateListSource candidateSource
        = ImageCandidateListSource::forArchiveDocument(session.location.archiveDocument());
    m_archiveListJob = m_candidateRepository.loadImages(
        this, candidateSource,
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
            invokeIfSet(m_callbacks.sourceResolved, session.location.imageUrl());
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
    if (!m_loadSession.has_value() || m_loadSession->id != request.id()
        || !sameNormalizedUrl(m_loadSession->location.imageUrl(), request.imageUrl())) {
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
    if (!m_callbacks.takePredecodedImage) {
        return false;
    }

    std::optional<PredecodedImage> predecoded
        = m_callbacks.takePredecodedImage(session.location.imageUrl());
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
    finishCurrentLoadSession(session, m_callbacks.error, error, errorString);
}

void ImageLoader::finishDecodedImage(ImageLoadSession session, DecodedImage image)
{
    finishCurrentLoadSession(session, m_callbacks.decodedImage, std::move(image));
}

void ImageLoader::finishPredecodedImage(ImageLoadSession session, PredecodedImage image)
{
    finishCurrentLoadSession(session, m_callbacks.predecodedImage, std::move(image));
}

template <typename Callback, typename... Args>
void ImageLoader::finishCurrentLoadSession(
    const ImageLoadSession &session, Callback &callback, Args &&...args)
{
    std::optional<ImageLoadSession> currentSession = takeCurrentLoadSession(session);
    if (!currentSession.has_value()) {
        return;
    }

    invokeIfSet(callback, std::move(*currentSession), std::forward<Args>(args)...);
}
}
