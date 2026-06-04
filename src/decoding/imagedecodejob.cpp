// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "async/imageasyncworker.h"
#include "async/imagecallback.h"
#include "thumbnailpreview.h"

#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
ImageDecodeJob::ImageDecodeJob(QObject *parent)
    : ImageDecodeJob(parent, Callbacks {})
{
}

ImageDecodeJob::ImageDecodeJob(QObject *parent, Callbacks callbacks)
    : ImageDecodeJob(parent, {}, std::move(callbacks))
{
}

ImageDecodeJob::ImageDecodeJob(QObject *parent, ImageDecodeDependencies dependencies)
    : ImageDecodeJob(parent, std::move(dependencies), Callbacks {})
{
}

ImageDecodeJob::ImageDecodeJob(
    QObject *parent, ImageDecodeDependencies dependencies, Callbacks callbacks)
    : QObject(parent)
    , m_dependencies(imageDecodeDependenciesWithDefaults(std::move(dependencies)))
    , m_callbacks(std::move(callbacks))
{
}

void ImageDecodeJob::start(ImageDecodeRequest request)
{
    cancel();
    if (request.isEmpty() || !m_dependencies.dataLoader || !m_dependencies.dataDecoder) {
        return;
    }

    const ImageDecodeJobTicket ticket = m_state.start(std::move(request));
    m_dataLoadJob = m_dependencies.dataLoader(
        this, ticket.request,
        [this, ticket](QByteArray data) mutable {
            ImageDecodeJobRuntimePlan plan = m_state.acceptLoadedData(ticket);
            auto *operation = std::get_if<StartImageDecodeOperation>(&plan.operation);
            if (operation == nullptr) {
                return;
            }

            startDecode(std::move(data), std::move(ticket), std::move(operation->request));
        },
        [this, ticket](const QString &errorString) {
            ImageDecodeJobRuntimePlan plan = m_state.acceptLoadError(ticket);
            const auto *operation = std::get_if<DeliverImageLoadErrorOperation>(&plan.operation);
            if (operation == nullptr) {
                return;
            }

            invokeIfSet(m_callbacks.loadError, operation->request, errorString);
        });
}

void ImageDecodeJob::cancel()
{
    m_state.cancel();
    m_dataLoadJob.cancel();
    m_thumbnailPreviewLookupJob.cancel();
}

bool ImageDecodeJob::hasActiveRequest() const { return m_state.hasActiveRequest(); }

void ImageDecodeJob::startDecode(
    QByteArray data, ImageDecodeJobTicket ticket, ImageDecodeRequest request)
{
    startThumbnailPreviewLookup(data, ticket, request);

    const ImageDataDecoder decoder = m_dependencies.dataDecoder;
    runAsyncWorker(
        this,
        [decoder, data = std::move(data), request = std::move(request)]() mutable {
            return decoder(data, request);
        },
        [this, ticket = std::move(ticket)](DecodedImageResult result) mutable {
            ImageDecodeJobRuntimePlan plan = m_state.acceptDecodeResult(ticket);
            auto *operation = std::get_if<DeliverImageDecodeResultOperation>(&plan.operation);
            if (operation == nullptr) {
                return;
            }

            invokeIfSet(m_callbacks.decoded, std::move(operation->request), std::move(result));
        });
}

void ImageDecodeJob::startThumbnailPreviewLookup(
    const QByteArray &data, ImageDecodeJobTicket ticket, const ImageDecodeRequest &request)
{
    if (!m_callbacks.thumbnailPreview || !m_dependencies.thumbnailPreviewLookupProvider) {
        return;
    }

    std::optional<XdgThumbnailPreviewRequest> previewRequest
        = xdgThumbnailPreviewRequestForDecodeData(data, request);
    if (!previewRequest.has_value()) {
        return;
    }

    std::optional<ThumbnailCacheLookupRequest> lookupRequest
        = xdgThumbnailPreviewCacheLookupRequest(*previewRequest);
    if (!lookupRequest.has_value()) {
        return;
    }

    m_thumbnailPreviewLookupJob = m_dependencies.thumbnailPreviewLookupProvider(this,
        std::move(*lookupRequest),
        [this, ticket = std::move(ticket), previewRequest = std::move(*previewRequest)](
            ThumbnailCacheLookupResult lookupResult) mutable {
            ImageDecodeJobRuntimePlan plan = m_state.acceptThumbnailPreview(ticket);
            const auto *operation
                = std::get_if<DeliverImageThumbnailPreviewOperation>(&plan.operation);
            if (operation == nullptr) {
                return;
            }

            XdgThumbnailPreviewResult previewResult
                = xdgThumbnailPreviewResult(previewRequest, std::move(lookupResult));
            std::optional<StaticDisplayImagePayload> payload
                = xdgThumbnailPreviewDisplayPayload(operation->request, std::move(previewResult));
            if (!payload.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.thumbnailPreview, operation->request, std::move(*payload));
        });
}
}
