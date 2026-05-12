// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecodecontroller.h"

#include "imagedocumentstate.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"

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
    m_coordinator->scheduleDisplayedImage(
        m_state.displayedImageLocation(), m_presentationController);
}

void ImageDocumentPredecodeController::cancel() { m_coordinator->cancel(); }

void ImageDocumentPredecodeController::clear() { m_coordinator->clear(); }

std::optional<PredecodedImage> ImageDocumentPredecodeController::tryTake(const QUrl &url) const
{
    return m_coordinator->tryTake(url);
}
}
