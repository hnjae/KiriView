// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeloadcontroller.h"

#include "decodedimageresult.h"
#include "imagedecodejob.h"

#include <optional>
#include <utility>

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
    m_loadState.cacheDisplayedImages(images);
}

void PredecodeLoadController::clearWindowUrls() { m_loadState.clearWindowUrls(); }

void PredecodeLoadController::startWindowLoads(PredecodeLoadWindow window)
{
    m_activeDecodes.cancel();
    m_loadState.startWindow(std::move(window));
    startNextLoads();
}

void PredecodeLoadController::startNextLoads()
{
    while (m_loadState.canStartMoreLoads(m_activeDecodes.size())) {
        std::optional<PredecodeLoadStart> load = m_loadState.takeNextLoad(m_activeDecodes.urls());
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
        return false;
    }

    auto *decodeJob = new ImageDecodeJob(m_parent, m_decodeDependencies,
        ImageDecodeJob::Callbacks {
            [this](ImageDecodeRequest request, DecodedImageResult result) {
                finishDecode(std::move(request), result);
            },
            [this](
                const ImageDecodeRequest &request, const QString &) { finishLoadError(request); },
        });
    const ImageDecodeRequest request = load.request;
    if (!m_activeDecodes.add(std::move(load.request), decodeJob)) {
        decodeJob->deleteLater();
        return false;
    }

    decodeJob->start(request);
    return true;
}

void PredecodeLoadController::finishLoadError(const ImageDecodeRequest &request)
{
    if (!m_activeDecodes.finish(request).has_value()) {
        return;
    }

    startNextLoads();
}

void PredecodeLoadController::finishDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    std::optional<ImageDecodeRequest> activeRequest = m_activeDecodes.finish(request);
    if (!activeRequest.has_value()) {
        return;
    }

    const auto *staticImage = decodedImageResultImageAs<StaticDecodedImage>(result);
    if (staticImage != nullptr) {
        m_loadState.cacheDecodedImage(*activeRequest, staticImage->staticImage);
    }

    startNextLoads();
}

void PredecodeLoadController::cancelBackgroundWork()
{
    m_activeDecodes.cancel();
    m_loadState.cancelBackgroundWork();
}

void PredecodeLoadController::clear()
{
    m_activeDecodes.cancel();
    m_loadState.clear();
}

std::optional<PredecodedImage> PredecodeLoadController::tryTake(const QUrl &url) const
{
    return m_loadState.tryTake(url);
}
}
