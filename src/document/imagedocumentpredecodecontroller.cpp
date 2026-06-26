// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecodecontroller.h"

#include "imagedocumentstate.h"
#include "location/imagedocumentlocation.h"
#include "predecode/imagepredecodecoordinator.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"

#include <optional>
#include <utility>
#include <vector>

namespace kiriview {
ImageDocumentPredecodeController::ImageDocumentPredecodeController(QObject* parent,
    ImageDocumentState& state, ImagePageSurfaceController& pageSurfaceController,
    ImagePresentationRuntime& presentationRuntime,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, qsizetype cacheByteBudget,
    CurrentPageNumberCallback currentPageNumber, PowerSaverProvider powerSaverProvider,
    bool ordinaryDirectMediaPredecodeEnabled, TimerScheduler timerScheduler,
    PredecodeThreadCountProvider threadCountProvider)
    : m_state(state)
    , m_pageSurfaceController(pageSurfaceController)
    , m_presentationRuntime(presentationRuntime)
    , m_coordinator(
          std::make_unique<ImagePredecodeCoordinator>(parent, std::move(candidateProvider),
              std::move(decodeDependencies), std::move(powerSaverProvider), cacheByteBudget,
              std::move(timerScheduler), std::move(threadCountProvider)))
    , m_currentPageNumber(std::move(currentPageNumber))
    , m_ordinaryDirectMediaPredecodeEnabled(ordinaryDirectMediaPredecodeEnabled)
{
}

ImageDocumentPredecodeController::~ImageDocumentPredecodeController() = default;

void ImageDocumentPredecodeController::scheduleAdjacentImagePredecode(
    std::optional<DisplayedPredecodeImage> secondaryImage)
{
    std::optional<StaticDisplayImagePayload> displayImage = m_pageSurfaceController.displayImage();
    if (!m_pageSurfaceController.hasImage() || m_state.displayedUrl().isEmpty()
        || !displayImage.has_value()) {
        m_coordinator->cancel();
        return;
    }

    if (!m_ordinaryDirectMediaPredecodeEnabled
        && !displayedLocationIsInsideOpenedCollectionScope(m_state.displayedImageLocation())) {
        m_coordinator->cancel();
        return;
    }

    DisplayedPredecodeImage primaryImage {
        m_state.displayedImageLocation(),
        m_pageSurfaceController.isPredecodeCacheable(),
        std::move(displayImage),
        m_state.embeddedMetadata(),
    };
    const DisplayedImageLocation currentLocation = primaryImage.location;
    std::vector<DisplayedPredecodeImage> displayedImages;
    displayedImages.push_back(std::move(primaryImage));
    if (secondaryImage.has_value()) {
        displayedImages.push_back(std::move(*secondaryImage));
    }

    m_coordinator->schedule(ImagePredecodeCoordinator::Context {
        currentLocation,
        std::move(displayedImages),
        m_presentationRuntime.firstDisplayDecodeContext(),
        m_currentPageNumber ? m_currentPageNumber() - 1 : -1,
        {},
    });
}

void ImageDocumentPredecodeController::cancel() { m_coordinator->cancel(); }

void ImageDocumentPredecodeController::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> ImageDocumentPredecodeController::findPredecodedImage(
    const QUrl& url) const
{
    return m_coordinator->findPredecodedImage(url);
}
}
