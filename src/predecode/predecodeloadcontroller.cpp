// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeloadcontroller.h"

#include "decoding/decodedimageresult.h"
#include "decoding/imagedecodejob.h"
#include "predecodelogging.h"

#include <QDebug>
#include <optional>
#include <utility>

namespace kiriview {
PredecodeLoadController::PredecodeLoadController(
    QObject *parent, ImageDecodeDependencies decodeDependencies, qsizetype cacheByteBudget)
    : m_parent(parent)
    , m_decodeDependencies(imageDecodeDependenciesWithDefaults(std::move(decodeDependencies)))
    , m_loadState(cacheByteBudget)
{
}

PredecodeLoadController::~PredecodeLoadController() = default;

void PredecodeLoadController::cacheDisplayedImages(
    const std::vector<DisplayedPredecodeImage> &images)
{
    m_loadState.cacheDisplayedImages(images);
}

void PredecodeLoadController::clearWindowUrls() { m_loadState.clearWindowUrls(); }

void PredecodeLoadController::startWindowLoads(PredecodeLoadWindow window)
{
    qCDebug(kiriviewPredecodeLog) << "predecode controller start window"
                                  << "generation" << window.generation << "primaryUrl"
                                  << window.primaryDisplayedUrl << "urls" << window.urls.size()
                                  << "parallelLimit" << window.parallelLimit;
    m_activeDecodes.cancel();
    m_loadState.startWindow(std::move(window));
    startNextLoads();
}

void PredecodeLoadController::startNextLoads()
{
    while (true) {
        std::optional<PredecodeLoadStart> load
            = m_loadState.takeNextLoad(m_activeDecodes.activeLoads());
        if (!load.has_value()) {
            return;
        }
        if (!startLoad(std::move(*load))) {
            return;
        }
    }
}

bool PredecodeLoadController::startLoad(PredecodeLoadStart load)
{
    if (load.request.isEmpty()) {
        qCDebug(kiriviewPredecodeLog) << "predecode decode job skipped"
                                      << "reason"
                                      << "empty-request";
        return false;
    }

    auto *decodeJob = new ImageDecodeJob(m_parent, m_decodeDependencies,
        ImageDecodeJob::Callbacks {
            [this](ImageDecodeRequest request, DecodedImageResult result) {
                finishDecode(std::move(request), result);
            },
            [this](
                const ImageDecodeRequest &request, const QString &) { finishLoadError(request); },
            {},
        });
    const ImageDecodeRequest request = load.request;
    if (!m_activeDecodes.add(std::move(load.request), decodeJob)) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode decode job skipped"
            << "reason"
            << "active-store-rejected"
            << "generation" << request.id() << "url" << request.imageUrl();
        decodeJob->deleteLater();
        return false;
    }

    qCDebug(kiriviewPredecodeLog) << "predecode decode job start"
                                  << "generation" << request.id() << "url" << request.imageUrl()
                                  << "activeLoads" << m_activeDecodes.size();
    decodeJob->start(request);
    return true;
}

void PredecodeLoadController::finishLoadError(const ImageDecodeRequest &request)
{
    if (!m_activeDecodes.finish(request).has_value()) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode load error ignored"
            << "reason"
            << "inactive-request"
            << "generation" << request.id() << "url" << request.imageUrl();
        return;
    }

    qCDebug(kiriviewPredecodeLog) << "predecode load error"
                                  << "generation" << request.id() << "url" << request.imageUrl();
    startNextLoads();
}

void PredecodeLoadController::finishDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    std::optional<ImageDecodeRequest> activeRequest = m_activeDecodes.finish(request);
    if (!activeRequest.has_value()) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode decode result ignored"
            << "reason"
            << "inactive-request"
            << "generation" << request.id() << "url" << request.imageUrl();
        return;
    }

    const auto *failure = decodedImageResultFailure(result);
    if (failure != nullptr) {
        qCDebug(kiriviewPredecodeLog)
            << "predecode decode failed"
            << "generation" << activeRequest->id() << "url" << activeRequest->imageUrl() << "error"
            << failure->errorString;
        startNextLoads();
        return;
    }

    const auto *staticImage = decodedImageResultImageAs<StaticDecodedImage>(result);
    if (staticImage != nullptr) {
        StaticDisplayImagePayload displayImage = staticImage->displayImage;
        qCDebug(kiriviewPredecodeLog)
            << "predecode decode finished"
            << "generation" << activeRequest->id() << "url" << activeRequest->imageUrl()
            << "originalSize" << displayImage.originalSize << "rasterSize"
            << displayImage.image.size() << "byteCost" << displayImage.byteCost();
        m_loadState.cacheDecodedImage(
            *activeRequest, std::move(displayImage), staticImage->embeddedMetadata);
    } else {
        qCDebug(kiriviewPredecodeLog)
            << "predecode decoded non-static image ignored"
            << "generation" << activeRequest->id() << "url" << activeRequest->imageUrl();
    }

    startNextLoads();
}

void PredecodeLoadController::cancelBackgroundWork()
{
    qCDebug(kiriviewPredecodeLog) << "predecode controller cancel background"
                                  << "activeLoads" << m_activeDecodes.size();
    m_activeDecodes.cancel();
    m_loadState.cancelBackgroundWork();
}

void PredecodeLoadController::clear()
{
    qCDebug(kiriviewPredecodeLog) << "predecode controller clear";
    m_activeDecodes.cancel();
    m_loadState.clear();
}

std::optional<PredecodedImage> PredecodeLoadController::findPredecodedImage(const QUrl &url) const
{
    return m_loadState.findPredecodedImage(url);
}
}
