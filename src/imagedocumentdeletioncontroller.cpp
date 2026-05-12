// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletioncontroller.h"

#include "imagedeletioncontroller.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"

namespace KiriView {
ImageDocumentDeletionController::ImageDocumentDeletionController(ImageDocumentState &state,
    ImagePresentationController &presentationController,
    ImageDeletionController &deletionController)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_deletionController(deletionController)
{
}

bool ImageDocumentDeletionController::inProgress() const
{
    return m_deletionController.inProgress();
}

void ImageDocumentDeletionController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!m_presentationController.hasImage()) {
        return;
    }

    m_deletionController.deleteDisplayedFile(m_state.displayedImageLocation(), mode);
}

void ImageDocumentDeletionController::cancel() { m_deletionController.cancel(); }
}
