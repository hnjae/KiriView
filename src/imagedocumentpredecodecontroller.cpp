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
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_coordinator(std::make_unique<ImagePredecodeCoordinator>(
          parent, std::move(candidateProvider), std::move(decodeDependencies)))
{
}

ImageDocumentPredecodeController::~ImageDocumentPredecodeController() = default;

void ImageDocumentPredecodeController::scheduleAdjacentImagePredecode()
{
    std::optional<StaticImagePayload> staticImage = m_presentationController.staticImage();
    if (!m_presentationController.hasImage() || m_state.displayedUrl().isEmpty()
        || !staticImage.has_value()) {
        m_coordinator->cancel();
        return;
    }

    m_coordinator->schedule(ImagePredecodeCoordinator::Context {
        m_state.displayedImageLocation(),
        m_presentationController.isPredecodeCacheable(),
        std::move(*staticImage),
        m_presentationController.firstDisplayDecodeContext(),
    });
}

void ImageDocumentPredecodeController::cancel() { m_coordinator->cancel(); }

void ImageDocumentPredecodeController::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> ImageDocumentPredecodeController::tryTake(const QUrl &url) const
{
    return m_coordinator->tryTake(url);
}
}
