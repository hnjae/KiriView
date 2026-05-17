// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeloadcontroller.h"

#include "decodedimageresult.h"
#include "imagedecodejob.h"

#include <optional>
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
PredecodeLoadController::PredecodeLoadController(QObject *parent)
    : PredecodeLoadController(parent, ImageDecodeDependencies {})
{
}

PredecodeLoadController::PredecodeLoadController(
    QObject *parent, ImageDecodeDependencies decodeDependencies)
    : m_parent(parent)
    , m_decodeDependencies(imageDecodeDependenciesWithDefaults(std::move(decodeDependencies)))
{
}

PredecodeLoadController::~PredecodeLoadController() = default;

void PredecodeLoadController::cacheDisplayedImages(
    const std::vector<DisplayedPredecodeImage> &images)
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

void PredecodeLoadController::clearWindowUrls() { m_cache.setWindowUrls({}); }

void PredecodeLoadController::startWindowLoads(PredecodeLoadWindow window)
{
    cancelBackgroundWork();
    m_parallelLimit = window.parallelLimit;
    m_firstDisplayContext = window.firstDisplayContext;
    m_cache.setWindowUrls(window.urls);
    cacheDisplayedImages(window.displayedImages);
    m_cache.enqueueMissingWindowLoads(
        window.primaryDisplayedUrl, window.archiveDocument, m_activeRequests.urls());

    startNextLoads(window.generation);
}

void PredecodeLoadController::startNextLoads(quint64 generation)
{
    if (m_parallelLimit == 0) {
        return;
    }

    while (m_activeRequests.size() < m_parallelLimit) {
        const std::optional<PredecodeRequest> request
            = m_cache.takeNextRequest(m_activeRequests.urls());
        if (!request.has_value()) {
            return;
        }
        if (!startLoad(request->url, request->archiveDocument, generation)) {
            return;
        }
    }
}

bool PredecodeLoadController::startLoad(
    const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation)
{
    const bool urlAvailable = url.isValid() && !url.isEmpty();
    const bool cached = urlAvailable && m_cache.hasImage(url);
    const bool inWindow = urlAvailable && m_cache.windowContains(url);
    if (!urlAvailable || m_activeRequests.size() >= m_parallelLimit || cached || !inWindow
        || m_activeRequests.containsUrl(url)) {
        return false;
    }

    ImageDecodeRequest request = ImageDecodeRequest::fromLocation(
        generation, DisplayedImageLocation::fromUrl(url, archiveDocument), m_firstDisplayContext);
    auto *decodeJob = new ImageDecodeJob(m_parent, m_decodeDependencies,
        ImageDecodeJob::Callbacks {
            [this](ImageDecodeRequest request, DecodedImageResult result) {
                finishDecode(std::move(request), result);
            },
            [this](
                const ImageDecodeRequest &request, const QString &) { finishLoadError(request); },
        });
    m_activeRequests.add(request, decodeJob);
    decodeJob->start(std::move(request));
    return true;
}

void PredecodeLoadController::finishLoadError(const ImageDecodeRequest &request)
{
    if (!m_activeRequests.finish(request).has_value()) {
        return;
    }

    startNextLoads(request.id());
}

void PredecodeLoadController::finishDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    std::optional<ImageDecodeRequest> activeRequest = m_activeRequests.finish(request);
    if (!activeRequest.has_value()) {
        return;
    }

    const auto *staticImage = decodedImageResultImageAs<StaticDecodedImage>(result);
    if (staticImage != nullptr) {
        m_cache.cacheImage(
            request.imageUrl(), activeRequest->archiveDocument(), staticImage->staticImage);
    }

    startNextLoads(request.id());
}

void PredecodeLoadController::cancelBackgroundWork()
{
    m_activeRequests.cancel();
    m_cache.clearQueuedLoads();
    m_parallelLimit = 1;
    m_firstDisplayContext = {};
}

void PredecodeLoadController::clear()
{
    cancelBackgroundWork();
    m_cache.clear();
}

std::optional<PredecodedImage> PredecodeLoadController::tryTake(const QUrl &url) const
{
    return m_cache.findImage(url);
}
}
