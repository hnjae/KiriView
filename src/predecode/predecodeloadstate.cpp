// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeloadstate.h"

#include "predecodelogging.h"

#include <QDebug>
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
PredecodeLoadState::PredecodeLoadState(qsizetype cacheByteBudget)
    : m_cache(cacheByteBudget)
{
}

void PredecodeLoadState::cacheDisplayedImages(const std::vector<DisplayedPredecodeImage> &images)
{
    qCDebug(kiriviewPredecodeLog) << "cache displayed images"
                                  << "count" << images.size();
    m_cache.setDisplayedUrls(displayedPredecodeImageUrls(images));
    for (const DisplayedPredecodeImage &image : images) {
        if (!image.isCacheable()) {
            qCDebug(kiriviewPredecodeLog) << "displayed image cache skipped"
                                          << "reason"
                                          << "not-cacheable"
                                          << "url" << image.location.imageUrl();
            continue;
        }

        m_cache.cacheDisplayedImage(
            true, image.location.imageUrl(), image.location.imagePageScope(), *image.staticImage);
    }
}

void PredecodeLoadState::clearWindowUrls() { m_cache.setWindowUrls({}); }

void PredecodeLoadState::startWindow(PredecodeLoadWindow window)
{
    qCDebug(kiriviewPredecodeLog) << "predecode load window"
                                  << "generation" << window.generation << "primaryUrl"
                                  << window.primaryDisplayedUrl << "urls" << window.urls.size()
                                  << "displayedImages" << window.displayedImages.size()
                                  << "parallelLimit" << window.parallelLimit;
    cancelBackgroundWork();
    m_activeWindow = ActiveWindow {
        window.firstDisplayContext,
        window.generation,
        window.parallelLimit,
    };
    m_cache.setWindowUrls(window.urls);
    cacheDisplayedImages(window.displayedImages);
    m_cache.enqueueMissingWindowLoads(
        window.primaryDisplayedUrl, window.imagePageScope, PredecodeActiveLoads {});
}

bool PredecodeLoadState::canStartMoreLoads(const PredecodeActiveLoads &activeLoads) const
{
    return m_activeWindow.has_value() && m_activeWindow->parallelLimit > 0
        && activeLoads.size() < m_activeWindow->parallelLimit;
}

std::optional<PredecodeLoadStart> PredecodeLoadState::takeNextLoad(
    const PredecodeActiveLoads &activeLoads)
{
    if (!canStartMoreLoads(activeLoads)) {
        return std::nullopt;
    }

    const std::optional<PredecodeRequest> request = m_cache.takeNextRequest(activeLoads);
    if (!request.has_value()) {
        qCDebug(kiriviewPredecodeLog) << "predecode next load unavailable"
                                      << "activeLoads" << activeLoads.size();
        return std::nullopt;
    }

    qCDebug(kiriviewPredecodeLog) << "predecode next load selected"
                                  << "generation" << m_activeWindow->generation << "url"
                                  << request->url << "activeLoads" << activeLoads.size();
    return PredecodeLoadStart {
        ImageDecodeRequest::fromLocation(m_activeWindow->generation,
            DisplayedImageLocation::fromUrl(request->url, request->imagePageScope),
            m_activeWindow->firstDisplayContext),
    };
}

void PredecodeLoadState::cacheDecodedImage(
    const ImageDecodeRequest &request, StaticImagePayload staticImage)
{
    qCDebug(kiriviewPredecodeLog) << "cache decoded predecode image"
                                  << "generation" << request.id() << "url" << request.imageUrl();
    m_cache.cacheImage(request.imageUrl(), request.imagePageScope(), std::move(staticImage));
}

void PredecodeLoadState::cancelBackgroundWork()
{
    qCDebug(kiriviewPredecodeLog) << "predecode load state cancel background";
    m_cache.clearQueuedLoads();
    m_activeWindow.reset();
}

void PredecodeLoadState::clear()
{
    cancelBackgroundWork();
    m_cache.clear();
}

std::optional<PredecodedImage> PredecodeLoadState::findPredecodedImage(const QUrl &url) const
{
    return m_cache.findImage(url);
}
}
