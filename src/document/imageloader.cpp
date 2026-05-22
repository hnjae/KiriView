// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "async/imagecallback.h"
#include "decoding/decodedimageresult.h"

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
    if (const DecodedImageFailure *failure = result.failure()) {
        finishDecodeRequestWithError(request, ImageLoadError::Generic, failure->errorString);
        return;
    }

    std::optional<DecodedImage> image = std::move(result).takeImage();
    if (!image.has_value()) {
        return;
    }

    std::optional<ImageLoadSession> session
        = m_sessionTracker.claimCurrentForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    finishDecodedImage(std::move(*session), std::move(*image));
}

void ImageLoader::finishImageLoadError(
    const ImageDecodeRequest &request, const QString &errorString)
{
    finishDecodeRequestWithError(request, ImageLoadError::Generic, errorString);
}

void ImageLoader::start(
    ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext)
{
    cancel();

    ImageLoadPlan plan = m_sessionTracker.start(std::move(request), firstDisplayContext);
    const ImageLoadSession session = std::move(plan.session);
    switch (plan.startEffect) {
    case ImageLoadStartEffect::DecodeImage:
        break;
    case ImageLoadStartEffect::LoadArchiveImageCandidates:
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
    if (!m_sessionTracker.isCurrent(session)) {
        return;
    }

    m_decodeJob.start(session.decodeRequest());
}

void ImageLoader::startArchiveLoad(ImageLoadSession session)
{
    const ImageCandidateListSource candidateSource
        = ImageCandidateListSource::forArchiveDocument(session.archiveDocument());
    m_archiveListJob = m_candidateRepository.loadImages(
        this, candidateSource,
        [this, session](std::vector<ImageNavigationCandidate> candidates) mutable {
            ImageArchiveCandidateCompletion completion
                = m_sessionTracker.completeArchiveCandidates(session, candidates);
            switch (completion.action) {
            case ImageArchiveCandidateCompletionAction::Ignored:
                return;
            case ImageArchiveCandidateCompletionAction::ReportEmptyArchive:
                invokeIfSet(m_callbacks.error, std::move(completion.session),
                    ImageLoadError::EmptyArchive, QString());
                return;
            case ImageArchiveCandidateCompletionAction::StartImageDecode:
                break;
            }

            invokeIfSet(m_callbacks.sourceResolved, completion.session);
            startImageLoad(std::move(completion.session));
        },
        [this, session](const QString &errorString) {
            std::optional<ImageLoadSession> currentSession = m_sessionTracker.claimCurrent(session);
            if (!currentSession.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.error, std::move(*currentSession), ImageLoadError::Generic,
                errorString);
        });
}

void ImageLoader::cancel()
{
    m_sessionTracker.cancel();
    m_decodeJob.cancel();
    m_archiveListJob.cancel();
}

bool ImageLoader::tryDisplayPredecodedImage(ImageLoadSession session)
{
    if (!m_callbacks.findPredecodedImage) {
        return false;
    }

    std::optional<PredecodedImage> predecoded = m_callbacks.findPredecodedImage(session.imageUrl());
    if (!predecoded.has_value()) {
        return false;
    }

    std::optional<ImageLoadSession> predecodedSession
        = m_sessionTracker.claimPredecodedImage(session, predecoded->location);
    if (!predecodedSession.has_value()) {
        return false;
    }

    finishPredecodedImage(std::move(*predecodedSession), std::move(*predecoded));
    return true;
}

void ImageLoader::finishDecodeRequestWithError(
    const ImageDecodeRequest &request, ImageLoadError error, const QString &errorString)
{
    std::optional<ImageLoadSession> session
        = m_sessionTracker.claimCurrentForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    invokeIfSet(m_callbacks.error, std::move(*session), error, errorString);
}

void ImageLoader::finishDecodedImage(ImageLoadSession session, DecodedImage image)
{
    invokeIfSet(m_callbacks.decodedImage, std::move(session), std::move(image));
}

void ImageLoader::finishPredecodedImage(ImageLoadSession session, PredecodedImage image)
{
    invokeIfSet(m_callbacks.predecodedImage, std::move(session), std::move(image));
}
}
