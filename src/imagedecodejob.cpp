// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "imageasyncworker.h"
#include "imagecallback.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDecodeJob::ImageDecodeJob(QObject *parent, ImageDecodeDependencies dependencies)
    : ImageDecodeJob(parent, std::move(dependencies), Callbacks {})
{
}

ImageDecodeJob::ImageDecodeJob(
    QObject *parent, ImageDecodeDependencies dependencies, Callbacks callbacks)
    : QObject(parent)
    , m_dataLoader(std::move(dependencies.dataLoader))
    , m_dataDecoder(std::move(dependencies.dataDecoder))
    , m_callbacks(std::move(callbacks))
{
}

void ImageDecodeJob::start(ImageDecodeRequest request)
{
    cancel();
    if (request.isEmpty() || !m_dataLoader || !m_dataDecoder) {
        return;
    }

    m_request = request;
    m_dataLoadJob = m_dataLoader(
        this, request,
        [this, request](QByteArray data) {
            if (!isCurrentRequest(request)) {
                return;
            }

            startDecode(std::move(data), request);
        },
        [this, request](const QString &errorString) {
            std::optional<ImageDecodeRequest> currentRequest = takeCurrentRequest(request);
            if (!currentRequest.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.loadError, *currentRequest, errorString);
        });
}

void ImageDecodeJob::cancel()
{
    m_request.reset();
    m_dataLoadJob.cancel();
}

bool ImageDecodeJob::hasActiveRequest() const { return m_request.has_value(); }

void ImageDecodeJob::startDecode(QByteArray data, ImageDecodeRequest request)
{
    const ImageDataDecoder decoder = m_dataDecoder;
    runAsyncWorker(
        this,
        [decoder, data = std::move(data), request]() mutable { return decoder(data, request); },
        [this, request](DecodedImageResult result) mutable {
            std::optional<ImageDecodeRequest> currentRequest = takeCurrentRequest(request);
            if (!currentRequest.has_value()) {
                return;
            }

            invokeIfSet(m_callbacks.decoded, std::move(*currentRequest), std::move(result));
        });
}

bool ImageDecodeJob::isCurrentRequest(const ImageDecodeRequest &request) const
{
    return m_request.has_value() && m_request->matches(request);
}

std::optional<ImageDecodeRequest> ImageDecodeJob::takeCurrentRequest(
    const ImageDecodeRequest &request)
{
    if (!isCurrentRequest(request)) {
        return std::nullopt;
    }

    std::optional<ImageDecodeRequest> currentRequest = std::move(m_request);
    m_request.reset();
    return currentRequest;
}
}
