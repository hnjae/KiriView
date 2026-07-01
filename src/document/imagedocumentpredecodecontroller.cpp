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
namespace {
    std::optional<std::vector<DisplayedPredecodeImage>> displayedPredecodeImages(
        ImageDocumentState& state, ImagePageSurfaceController& pageSurfaceController,
        std::optional<DisplayedPredecodeImage> secondaryImage)
    {
        std::optional<StaticDisplayImagePayload> displayImage
            = pageSurfaceController.displayImage();
        if (!pageSurfaceController.hasImage() || state.displayedUrl().isEmpty()
            || !displayImage.has_value()) {
            return std::nullopt;
        }

        DisplayedPredecodeImage primaryImage {
            state.displayedImageLocation(),
            pageSurfaceController.isPredecodeCacheable(),
            std::move(displayImage),
            state.embeddedMetadata(),
        };
        std::vector<DisplayedPredecodeImage> displayedImages;
        displayedImages.push_back(std::move(primaryImage));
        if (secondaryImage.has_value()) {
            displayedImages.push_back(std::move(*secondaryImage));
        }

        return displayedImages;
    }

    std::optional<DisplayedImageLocation> predecodeLocationForTarget(
        const ImageDocumentPageTarget& target, const DisplayedImageLocation& currentLocation)
    {
        if (target.kind != ImageDocumentPageKind::Image || target.url.isEmpty()) {
            return std::nullopt;
        }

        if (displayedLocationIsInsideOpenedCollectionScope(currentLocation)
            && openedCollectionScopeContainsUrl(
                currentLocation.openedCollectionScope(), target.url)) {
            return DisplayedImageLocation::fromOpenedCollectionScope(
                target.url, currentLocation.openedCollectionScope());
        }

        return DisplayedImageLocation::fromUrl(target.url);
    }

    bool predecodeScopeAllowed(
        const DisplayedImageLocation& location, bool ordinaryDirectMediaPredecodeEnabled)
    {
        return ordinaryDirectMediaPredecodeEnabled
            || displayedLocationIsInsideOpenedCollectionScope(location);
    }
}

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
    std::optional<std::vector<DisplayedPredecodeImage>> displayedImages
        = displayedPredecodeImages(m_state, m_pageSurfaceController, std::move(secondaryImage));
    if (!displayedImages.has_value()) {
        m_coordinator->cancel();
        return;
    }

    const DisplayedImageLocation currentLocation = m_state.displayedImageLocation();
    if (!predecodeScopeAllowed(currentLocation, m_ordinaryDirectMediaPredecodeEnabled)) {
        m_coordinator->cancel();
        return;
    }

    m_coordinator->schedule(ImagePredecodeCoordinator::Context {
        currentLocation,
        std::move(*displayedImages),
        m_presentationRuntime.firstDisplayDecodeContext(),
        m_currentPageNumber ? m_currentPageNumber() - 1 : -1,
        {},
    });
}

void ImageDocumentPredecodeController::scheduleImageNavigationTargetPredecode(
    const ImageDocumentPageTarget& target, int targetPageIndex,
    std::optional<DisplayedPredecodeImage> secondaryImage)
{
    const std::optional<DisplayedImageLocation> targetLocation
        = predecodeLocationForTarget(target, m_state.displayedImageLocation());
    if (!targetLocation.has_value()) {
        return;
    }

    if (!predecodeScopeAllowed(*targetLocation, m_ordinaryDirectMediaPredecodeEnabled)) {
        return;
    }

    std::optional<std::vector<DisplayedPredecodeImage>> displayedImages
        = displayedPredecodeImages(m_state, m_pageSurfaceController, std::move(secondaryImage));
    if (!displayedImages.has_value()) {
        return;
    }

    m_coordinator->schedule(ImagePredecodeCoordinator::Context {
        *targetLocation,
        std::move(*displayedImages),
        m_presentationRuntime.firstDisplayDecodeContext(),
        targetPageIndex,
        {},
        true,
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
