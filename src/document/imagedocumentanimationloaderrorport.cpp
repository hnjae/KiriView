// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentanimationloaderrorport.h"

#include "imageopencontroller.h"

namespace kiriview {
ImageDocumentAnimationLoadErrorPort::ImageDocumentAnimationLoadErrorPort(
    ImageOpenController *openController)
    : m_openController(openController)
{
}

void ImageDocumentAnimationLoadErrorPort::setOpenController(ImageOpenController *openController)
{
    m_openController = openController;
}

void ImageDocumentAnimationLoadErrorPort::finishAnimationLoadWithError(
    const QString &errorString) const
{
    if (m_openController != nullptr) {
        m_openController->finishAnimationLoadWithError(errorString);
    }
}
}
