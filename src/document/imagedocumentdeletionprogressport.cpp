// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionprogressport.h"

#include "imagedocumentdeletioncontroller.h"

namespace kiriview {
ImageDocumentDeletionProgressPort::ImageDocumentDeletionProgressPort(
    const ImageDocumentDeletionController *deletionController)
    : m_deletionController(deletionController)
{
}

bool ImageDocumentDeletionProgressPort::inProgress() const
{
    return m_deletionController != nullptr && m_deletionController->inProgress();
}
}
