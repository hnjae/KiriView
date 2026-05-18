// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeloadstate.h"

#include <utility>

namespace {
std::vector<QUrl> displayedPredecodeImageUrls(
    const std::vector<KiriView::DisplayedPredecodeImage> &images)
{
    std::vector<QUrl> urls;
    urls.reserve(images.size());
    for (const KiriView::DisplayedPredecodeImage &image : images) {
        if (image.hasLocation()) {
            urls.push_back(image.location.imageUrl());
        }
    }

    return urls;
}
}

namespace KiriView {
void PredecodeLoadState::cacheDisplayedImages(const std::vector<DisplayedPredecodeImage> &images)
{
    m_cache.setDisplayedUrls(displayedPredecodeImageUrls(images));
    for (const DisplayedPredecodeImage &image : images) {
        if (!image.isCacheable()) {
            continue;
        }

        m_cache.cacheDisplayedImage(
            true, image.location.imageUrl(), image.location.archiveDocument(), *image.staticImage);
    }
}

void PredecodeLoadState::clearWindowUrls() { m_cache.setWindowUrls({}); }

void PredecodeLoadState::startWindow(PredecodeLoadWindow window)
{
    cancelBackgroundWork();
    m_activeWindow = ActiveWindow {
        window.firstDisplayContext,
        window.generation,
        window.parallelLimit,
    };
    m_cache.setWindowUrls(window.urls);
    cacheDisplayedImages(window.displayedImages);
    m_cache.enqueueMissingWindowLoads(
        window.primaryDisplayedUrl, window.archiveDocument, m_activeRequests.urls());
}

bool PredecodeLoadState::canStartMoreLoads() const
{
    return m_activeWindow.has_value() && m_activeWindow->parallelLimit > 0
        && m_activeRequests.size() < m_activeWindow->parallelLimit;
}

std::optional<PredecodeLoadStart> PredecodeLoadState::takeNextLoad()
{
    if (!canStartMoreLoads()) {
        return std::nullopt;
    }

    const std::optional<PredecodeRequest> request
        = m_cache.takeNextRequest(m_activeRequests.urls());
    if (!request.has_value()) {
        return std::nullopt;
    }

    return PredecodeLoadStart {
        ImageDecodeRequest::fromLocation(m_activeWindow->generation,
            DisplayedImageLocation::fromUrl(request->url, request->archiveDocument),
            m_activeWindow->firstDisplayContext),
    };
}

void PredecodeLoadState::addActiveLoad(ImageDecodeRequest request, ImageDecodeJob *decodeJob)
{
    m_activeRequests.add(std::move(request), decodeJob);
}

std::optional<ImageDecodeRequest> PredecodeLoadState::finishActiveLoad(
    const ImageDecodeRequest &request)
{
    return m_activeRequests.finish(request);
}

void PredecodeLoadState::cacheDecodedImage(
    const ImageDecodeRequest &request, StaticImagePayload staticImage)
{
    m_cache.cacheImage(request.imageUrl(), request.archiveDocument(), std::move(staticImage));
}

void PredecodeLoadState::cancelBackgroundWork()
{
    m_activeRequests.cancel();
    m_cache.clearQueuedLoads();
    m_activeWindow.reset();
}

void PredecodeLoadState::clear()
{
    cancelBackgroundWork();
    m_cache.clear();
}

std::optional<PredecodedImage> PredecodeLoadState::tryTake(const QUrl &url) const
{
    return m_cache.findImage(url);
}
}
