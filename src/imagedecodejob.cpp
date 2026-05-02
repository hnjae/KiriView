// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "imageurl.h"

#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <utility>

namespace KiriView {
ImageDecodeJob::ImageDecodeJob(QObject *parent, DataLoader dataLoader, DataDecoder dataDecoder)
    : QObject(parent)
    , m_dataLoader(std::move(dataLoader))
    , m_dataDecoder(std::move(dataDecoder))
{
}

void ImageDecodeJob::setDecodedCallback(DecodedCallback callback)
{
    m_decoded = std::move(callback);
}

void ImageDecodeJob::setLoadErrorCallback(LoadErrorCallback callback)
{
    m_loadError = std::move(callback);
}

void ImageDecodeJob::start(ImageDecodeRequest request)
{
    cancel();
    if (request.imageUrl.isEmpty() || !m_dataLoader || !m_dataDecoder) {
        return;
    }

    m_request = request;
    m_dataLoadJob = m_dataLoader(
        this, request.imageUrl,
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
            if (m_loadError) {
                m_loadError(request, errorString);
            }
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
    const QPointer<ImageDecodeJob> decodeJob(this);
    const DataDecoder decoder = m_dataDecoder;
    QThreadPool::globalInstance()->start(
        QRunnable::create([decodeJob, decoder, data = std::move(data), request]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decoder(data));
            if (decodeJob == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                decodeJob.data(),
                [decodeJob, request, result]() mutable {
                    if (decodeJob == nullptr || !decodeJob->isCurrentRequest(request)) {
                        return;
                    }

                    decodeJob->clearRequest(request);
                    if (decodeJob->m_decoded) {
                        decodeJob->m_decoded(request, std::move(result));
                    }
                },
                Qt::QueuedConnection);
        }));
}

bool ImageDecodeJob::isCurrentRequest(const ImageDecodeRequest &request) const
{
    return m_request.has_value() && m_request->id == request.id
        && sameNormalizedUrl(m_request->imageUrl, request.imageUrl);
}

void ImageDecodeJob::clearRequest(const ImageDecodeRequest &request)
{
    if (isCurrentRequest(request)) {
        m_request.reset();
    }
}
}
