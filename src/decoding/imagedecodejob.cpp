// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "async/imageasyncworker.h"
#include "async/imagecallback.h"

#include <optional>
#include <utility>

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
            if (!m_state.beginDecode(ticket).has_value()) {
                return;
            }

            startDecode(std::move(data), std::move(ticket));
        },
        [this, ticket](const QString &errorString) {
            std::optional<ImageDecodeRequest> currentRequest = m_state.claimLoadError(ticket);
            if (!currentRequest.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.loadError, *currentRequest, errorString);
        });
}

void ImageDecodeJob::cancel()
{
    m_state.cancel();
    m_dataLoadJob.cancel();
}

bool ImageDecodeJob::hasActiveRequest() const { return m_state.hasActiveRequest(); }

void ImageDecodeJob::startDecode(QByteArray data, ImageDecodeJobTicket ticket)
{
    const ImageDataDecoder decoder = m_dependencies.dataDecoder;
    runAsyncWorker(
        this,
        [decoder, data = std::move(data), request = ticket.request]() mutable {
            return decoder(data, request);
        },
        [this, ticket = std::move(ticket)](DecodedImageResult result) mutable {
            std::optional<ImageDecodeRequest> currentRequest = m_state.claimDecodeResult(ticket);
            if (!currentRequest.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.decoded, std::move(*currentRequest), std::move(result));
        });
}
}
