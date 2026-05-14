// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletioncontroller.h"

#include "imagecallback.h"
#include "imagedeletioncontroller.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"

#include <utility>
#include <variant>

namespace KiriView {
ImageDocumentDeletionController::ImageDocumentDeletionController(QObject *parent,
    ImageDocumentState &state, ImagePresentationController &presentationController,
    ImageNavigationCandidateProvider candidateProvider, FileOperationProvider fileOperationProvider,
    Callbacks callbacks)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
{
    m_deletionController = std::make_unique<ImageDeletionController>(parent,
        std::move(candidateProvider), std::move(fileOperationProvider),
        ImageDeletionController::Callbacks {
            [this]() { invokeIfSet(m_callbacks.inProgressChanged); },
            [this](ImageDeletionEffect effect) { dispatch(std::move(effect)); },
        });
}

ImageDocumentDeletionController::~ImageDocumentDeletionController() = default;

bool ImageDocumentDeletionController::inProgress() const
{
    return m_deletionController->inProgress();
}

void ImageDocumentDeletionController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!m_presentationController.hasImage()) {
        return;
    }

    m_deletionController->deleteDisplayedFile(m_state.displayedImageLocation(), mode);
}

void ImageDocumentDeletionController::cancel() { m_deletionController->cancel(); }

void ImageDocumentDeletionController::dispatch(ImageDeletionEffect effect)
{
    std::visit([this](const auto &payload) { dispatchPayload(payload); }, effect.payload);
}

void ImageDocumentDeletionController::dispatchPayload(const ClearDeletedImageAfterDeletionEffect &)
{
    reportDocumentEffect(ImageDocumentEffect::clearDeletedImage());
}

void ImageDocumentDeletionController::dispatchPayload(
    const OpenImageDeletionFallbackEffect &payload)
{
    reportDocumentEffect(ImageDocumentEffect::openUrl(payload.url));
}

void ImageDocumentDeletionController::dispatchPayload(
    const OpenContainerImageDeletionFallbackEffect &payload)
{
    reportDocumentEffect(
        ImageDocumentEffect::containerImageSelected(payload.imageUrl, payload.containerUrl));
}

void ImageDocumentDeletionController::dispatchPayload(
    const ReportImageDeletionFailureEffect &payload)
{
    invokeIfSet(m_callbacks.failed, payload.errorString);
}

void ImageDocumentDeletionController::reportDocumentEffect(ImageDocumentEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}
}
