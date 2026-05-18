// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecodecontroller.h"

#include "imagedocumentstate.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"

#include <optional>
#include <utility>

namespace KiriView {
ImageDocumentPredecodeController::ImageDocumentPredecodeController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies,
    CurrentPageNumberCallback currentPageNumber, PowerSaverProvider powerSaverProvider)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_coordinator(
          std::make_unique<ImagePredecodeCoordinator>(parent, std::move(candidateProvider),
              std::move(decodeDependencies), std::move(powerSaverProvider)))
    , m_currentPageNumber(std::move(currentPageNumber))
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

    m_coordinator->schedule(ImagePredecodeCoordinator::Context {
        DisplayedPredecodeImage {
            m_state.displayedImageLocation(),
            m_presentationController.isPredecodeCacheable(),
            std::move(staticImage),
        },
        std::move(secondaryImage),
        m_presentationController.firstDisplayDecodeContext(),
        m_currentPageNumber ? m_currentPageNumber() - 1 : -1,
    });
}

void ImageDocumentPredecodeController::cancel() { m_coordinator->cancel(); }

void ImageDocumentPredecodeController::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> ImageDocumentPredecodeController::tryTake(const QUrl &url) const
{
    return m_coordinator->tryTake(url);
}
}
