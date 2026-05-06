// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "imageasyncworker.h"
#include "imagecallback.h"

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
            if (!isCurrentRequest(request)) {
                return;
            }

            clearRequest(request);
            invokeIfSet(m_callbacks.loadError, request, errorString);
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
            if (!isCurrentRequest(request)) {
                return;
            }

            clearRequest(request);
            invokeIfSet(m_callbacks.decoded, request, std::move(result));
        });
}

bool ImageDecodeJob::isCurrentRequest(const ImageDecodeRequest &request) const
{
    return m_request.has_value() && m_request->matches(request);
}

void ImageDecodeJob::clearRequest(const ImageDecodeRequest &request)
{
    if (isCurrentRequest(request)) {
        m_request.reset();
    }
}
}
