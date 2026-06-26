// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloader.h"

#include "async/imagecallback.h"
#include "decoding/decodedimageresult.h"
#include <optional>
#include <utility>
#include <vector>

namespace {
kiriview::ImageLoadFailure imageLoadFailure(const kiriview::ImageLoadSession& session,
    kiriview::ImageLoadFailureKind kind, QString userMessage, QString diagnosticDetail)
{
    return kiriview::ImageLoadFailure {
        session.imageUrl(),
        session.id(),
        kind,
        std::move(userMessage),
        std::move(diagnosticDetail),
        kiriview::ImageLoadFailureSeverity::Error,
        false,
    };
}

kiriview::ImageLoadFailure imageLoadFailure(const kiriview::ImageLoadSession& session,
    kiriview::ImageLoadFailureKind kind, const QString& errorString)
{
    return imageLoadFailure(session, kind, errorString, errorString);
}
}

namespace kiriview {
ImageLoader::ImageLoader(QObject* parent)
    : ImageLoader(parent, ImageDocumentPageCandidateProvider {}, ImageDecodeDependencies {})
{
}

ImageLoader::ImageLoader(QObject* parent, Callbacks callbacks)
    : ImageLoader(parent, ImageDocumentPageCandidateProvider {}, ImageDecodeDependencies {},
          std::move(callbacks))
{
}

ImageLoader::ImageLoader(QObject* parent, ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies)
    : ImageLoader(parent, std::move(candidateProvider), std::move(decodeDependencies), {})
{
}

ImageLoader::ImageLoader(QObject* parent, ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, Callbacks callbacks)
    : QObject(parent)
    , m_callbacks(std::move(callbacks))
    , m_decodeJob(this, std::move(decodeDependencies),
          ImageDecodeJob::Callbacks {
              [this](ImageDecodeRequest request, DecodedImageResult result) {
                  finishDecodeResult(std::move(request), std::move(result));
              },
              [this](const ImageDecodeRequest& request, const QString& errorString) {
                  finishImageLoadError(request, errorString);
              },
              [this](const ImageDecodeRequest& request, StaticDisplayImagePayload preview) {
                  finishThumbnailPreview(request, std::move(preview));
              },
          })
    , m_candidateRepository(std::move(candidateProvider))
{
}

void ImageLoader::finishDecodeResult(ImageDecodeRequest request, DecodedImageResult result)
{
    if (const DecodedImageFailure* failure = result.failure()) {
        finishDecodeRequestWithError(request, ImageLoadFailureKind::Decode, failure->errorString);
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
    const ImageDecodeRequest& request, const QString& errorString)
{
    finishDecodeRequestWithError(request, ImageLoadFailureKind::DataLoad, errorString);
}

void ImageLoader::finishThumbnailPreview(
    const ImageDecodeRequest& request, StaticDisplayImagePayload preview)
{
    std::optional<ImageLoadSession> session = m_sessionTracker.currentForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    finishThumbnailPreview(std::move(*session), std::move(preview));
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
    case ImageLoadStartEffect::LoadOpenedCollectionScopeCandidates:
        startOpenedCollectionLoad(session);
        return;
    }

    if (tryReportUnsupportedOpenedCollectionVideo(session)) {
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

void ImageLoader::startOpenedCollectionLoad(ImageLoadSession session)
{
    const ImageDocumentPageCandidateListSource candidateSource
        = ImageDocumentPageCandidateListSource::forOpenedCollectionScope(
            session.openedCollectionScope());
    m_openedCollectionCandidateLoadJob = m_candidateRepository.loadImages(
        this, candidateSource,
        [this, session](std::vector<ImageDocumentPageCandidate> candidates) mutable {
            OpenedCollectionCandidateCompletion completion
                = m_sessionTracker.completeOpenedCollectionCandidates(session, candidates);
            switch (completion.action) {
            case OpenedCollectionCandidateCompletionAction::Ignored:
                return;
            case OpenedCollectionCandidateCompletionAction::ReportEmptyOpenedCollection:
                invokeIfSet(m_callbacks.error, completion.session,
                    imageLoadFailure(completion.session,
                        ImageLoadFailureKind::EmptyOpenedCollection, QString(), QString()));
                return;
            case OpenedCollectionCandidateCompletionAction::ReportUnsupportedOpenedCollectionVideo:
                invokeIfSet(m_callbacks.sourceResolved, completion.session);
                invokeIfSet(
                    m_callbacks.unsupportedOpenedCollectionVideo, std::move(completion.session));
                return;
            case OpenedCollectionCandidateCompletionAction::StartImageDecode:
                break;
            }

            invokeIfSet(m_callbacks.sourceResolved, completion.session);
            startImageLoad(std::move(completion.session));
        },
        [this, session](const QString& errorString) {
            std::optional<ImageLoadSession> currentSession = m_sessionTracker.claimCurrent(session);
            if (!currentSession.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.error, *currentSession,
                imageLoadFailure(
                    *currentSession, ImageLoadFailureKind::OpenedCollectionLoad, errorString));
        });
}

void ImageLoader::cancel()
{
    m_sessionTracker.cancel();
    m_decodeJob.cancel();
    m_openedCollectionCandidateLoadJob.cancel();
}

bool ImageLoader::tryReportUnsupportedOpenedCollectionVideo(ImageLoadSession session)
{
    if (session.openedCollectionScope().isEmpty()) {
        return false;
    }

    if (session.kind() != ImageDocumentPageKind::Video) {
        return false;
    }

    std::optional<ImageLoadSession> currentSession = m_sessionTracker.claimCurrent(session);
    if (!currentSession.has_value()) {
        return false;
    }

    invokeIfSet(m_callbacks.unsupportedOpenedCollectionVideo, std::move(*currentSession));
    return true;
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
    if (predecoded->location != session.location()) {
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
    const ImageDecodeRequest& request, ImageLoadFailureKind kind, const QString& errorString)
{
    std::optional<ImageLoadSession> session
        = m_sessionTracker.claimCurrentForDecodeRequest(request);
    if (!session.has_value()) {
        return;
    }

    invokeIfSet(m_callbacks.error, *session, imageLoadFailure(*session, kind, errorString));
}

void ImageLoader::finishDecodedImage(ImageLoadSession session, DecodedImage image)
{
    invokeIfSet(m_callbacks.decodedImage, std::move(session), std::move(image));
}

void ImageLoader::finishPredecodedImage(ImageLoadSession session, PredecodedImage image)
{
    invokeIfSet(m_callbacks.predecodedImage, std::move(session), std::move(image));
}

void ImageLoader::finishThumbnailPreview(
    ImageLoadSession session, StaticDisplayImagePayload preview)
{
    invokeIfSet(m_callbacks.thumbnailPreview, std::move(session), std::move(preview));
}
}
