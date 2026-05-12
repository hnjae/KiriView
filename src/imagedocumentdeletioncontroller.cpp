// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletioncontroller.h"

#include "imagecallback.h"
#include "imagedeletioncontroller.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"

#include <utility>

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
            [this]() { invokeIfSet(m_callbacks.clearDeletedImage); },
            [this](const QUrl &url) { report(ImageDocumentEffect::openUrl(url)); },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                report(ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
            },
            [this](const QString &errorString) { invokeIfSet(m_callbacks.failed, errorString); },
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

void ImageDocumentDeletionController::report(ImageDocumentEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}
}
