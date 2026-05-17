// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "decodedimageresult.h"
#include "imagecallback.h"

#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
ImageLoader::ImageLoader(QObject *parent)
    : ImageLoader(parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {})
{
}

ImageLoader::ImageLoader(QObject *parent, Callbacks callbacks)
    : ImageLoader(parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {},
          std::move(callbacks))
{
}

ImageLoader::ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies)
    : ImageLoader(parent, std::move(candidateProvider), std::move(decodeDependencies), {})
{
}

ImageLoader::ImageLoader(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_decodeJob(this, std::move(decodeDependencies),
          ImageDecodeJob::Callbacks {
              [this](ImageDecodeRequest request, DecodedImageResult result) {
                  finishDecodeResult(std::move(request), std::move(result));
              },
              [this](const ImageDecodeRequest &request, const QString &errorString) {
                  finishImageLoadError(request, errorString);
              },
          })
    , m_candidateRepository(std::move(candidateProvider))
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

    ImageLoadPlan plan = m_sessionTracker.start(std::move(request), firstDisplayContext);
    const ImageLoadSession session = std::move(plan.session);
    if (plan.requiresArchiveListing) {
        startArchiveLoad(session);
        return;
    }

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

    m_decodeJob.start(ImageDecodeRequest::fromLocation(
        session.id, session.location, m_sessionTracker.firstDisplayContext()));
}

void ImageLoader::startArchiveLoad(ImageLoadSession session)
{
    const ImageCandidateListSource candidateSource
        = ImageCandidateListSource::forArchiveDocument(session.location.archiveDocument());
    m_archiveListJob = m_candidateRepository.loadImages(
        this, candidateSource,
        [this, session](std::vector<ImageNavigationCandidate> candidates) mutable {
            if (candidates.empty()) {
                if (isCurrentLoadSession(session)) {
                    finishLoadWithError(session, ImageLoadError::EmptyArchive, QString());
                }
                return;
            }

            std::optional<ImageLoadSession> resolvedSession
                = m_sessionTracker.resolveCurrentArchiveImage(session, candidates.front().url);
            if (!resolvedSession.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.sourceResolved, resolvedSession->location.imageUrl());
            startImageLoad(*resolvedSession);
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
    m_sessionTracker.cancel();
    m_decodeJob.cancel();
    m_archiveListJob.cancel();
}

std::optional<ImageLoadSession> ImageLoader::currentLoadSessionForDecodeRequest(
    const ImageDecodeRequest &request) const
{
    return m_sessionTracker.currentForDecodeRequest(request);
}

bool ImageLoader::isCurrentLoadSession(const ImageLoadSession &session) const
{
    return m_sessionTracker.isCurrent(session);
}

std::optional<ImageLoadSession> ImageLoader::takeCurrentLoadSession(const ImageLoadSession &session)
{
    return m_sessionTracker.takeCurrent(session);
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

    std::optional<ImageLoadSession> predecodedSession
        = m_sessionTracker.replaceCurrentLocation(session, predecoded->location);
    if (!predecodedSession.has_value()) {
        return false;
    }

    finishPredecodedImage(*predecodedSession, std::move(*predecoded));
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
