// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncdependencies.h"

#include "imageiojobs.h"
#include "kiriimagedecoder.h"

#include <utility>

namespace {
KiriView::ImageIoJob loadImageData(QObject *receiver, KiriView::ImageDecodeRequest request,
    KiriView::ImageDecodeJob::DataCallback callback,
    KiriView::ImageDecodeJob::ErrorCallback errorCallback)
{
    return KiriView::startStoredImageDataLoad(
        receiver, std::move(request), std::move(callback), std::move(errorCallback));
}

KiriView::DecodedImageResult decodeImageDataWithDefaults(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    return KiriView::decodeImageData(data, request);
}
}

namespace KiriView {
ImageAsyncDependencies defaultImageAsyncDependencies()
{
    return ImageAsyncDependencies {
        defaultImageNavigationCandidateProvider(),
        loadImageData,
        decodeImageDataWithDefaults,
    };
}
}
