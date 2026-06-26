// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentprimarypageslotport.h"

#include "presentation/imagespreadpresentationcontroller.h"

namespace kiriview {
ImageDocumentPrimaryPageSlotPort::ImageDocumentPrimaryPageSlotPort(
    ImageSpreadPresentationController* spreadController)
    : m_spreadController(spreadController)
{
}

void ImageDocumentPrimaryPageSlotPort::commit(const DisplayedImageLocation& location) const
{
    if (m_spreadController != nullptr) {
        m_spreadController->commitPrimaryPageSlot(location);
    }
}

void ImageDocumentPrimaryPageSlotPort::clear() const
{
    if (m_spreadController != nullptr) {
        m_spreadController->clearPrimaryPageSlot();
    }
}
}
