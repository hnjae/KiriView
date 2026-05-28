// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecodecontroller.h"

#include "imagedocumentstate.h"
#include "location/imagedocumentlocation.h"
#include "predecode/imagepredecodecoordinator.h"
#include "presentation/imagepresentationcontroller.h"

#include <optional>
#include <utility>
#include <vector>

namespace KiriView {
ImageDocumentPredecodeController::ImageDocumentPredecodeController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, qsizetype cacheByteBudget,
    CurrentPageNumberCallback currentPageNumber, PowerSaverProvider powerSaverProvider,
    bool ordinaryDirectMediaPredecodeEnabled)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_coordinator(
          std::make_unique<ImagePredecodeCoordinator>(parent, std::move(candidateProvider),
              std::move(decodeDependencies), std::move(powerSaverProvider), cacheByteBudget))
    , m_currentPageNumber(std::move(currentPageNumber))
    , m_ordinaryDirectMediaPredecodeEnabled(ordinaryDirectMediaPredecodeEnabled)
{
}

ImageDocumentPredecodeController::~ImageDocumentPredecodeController() = default;

void ImageDocumentPredecodeController::scheduleAdjacentImagePredecode(
    std::optional<DisplayedPredecodeImage> secondaryImage)
{
    std::optional<StaticImagePayload> staticImage = m_presentationController.staticImage();
    if (!m_presentationController.hasImage() || m_state.displayedUrl().isEmpty()
        || !staticImage.has_value()) {
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
        m_presentationController.isPredecodeCacheable(),
        std::move(staticImage),
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
        m_presentationController.firstDisplayDecodeContext(),
        m_currentPageNumber ? m_currentPageNumber() - 1 : -1,
        {},
    });
}

void ImageDocumentPredecodeController::cancel() { m_coordinator->cancel(); }

void ImageDocumentPredecodeController::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> ImageDocumentPredecodeController::findPredecodedImage(
    const QUrl &url) const
{
    return m_coordinator->findPredecodedImage(url);
}
}
