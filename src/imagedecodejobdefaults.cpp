// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include "imageiojobs.h"

#include <utility>

namespace {
KiriView::ImageIoJob defaultImageDataLoader(QObject *receiver, QUrl imageUrl,
    KiriView::ImageDecodeJob::DataCallback callback,
    KiriView::ImageDecodeJob::ErrorCallback errorCallback)
{
    return KiriView::startStoredImageDataLoad(
        receiver, std::move(imageUrl), std::move(callback), std::move(errorCallback));
}

KiriView::DecodedImageResult defaultImageDataDecoder(const QByteArray &data)
{
    return KiriView::decodeImageData(data);
}
}

namespace KiriView {
ImageDecodeJob::ImageDecodeJob(QObject *parent)
    : ImageDecodeJob(parent, defaultImageDataLoader, defaultImageDataDecoder)
{
}
}
