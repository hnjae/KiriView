// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportscanstate.h"

namespace kiriview {
void ImageViewportScanState::requestNextDisplayedImageFinalScanStart()
{
    m_pendingFinalScanStart = true;
}

ImageViewportScanStart ImageViewportScanState::beginDisplayedImage()
{
    m_displayedImageScanStart
        = m_pendingFinalScanStart ? ImageViewportScanStart::Final : ImageViewportScanStart::Initial;
    m_pendingFinalScanStart = false;
    return m_displayedImageScanStart;
}

void ImageViewportScanState::cancelPendingDisplayedImageStart() { m_pendingFinalScanStart = false; }

ImageViewportScanStart ImageViewportScanState::displayedImageScanStart() const
{
    return m_displayedImageScanStart;
}

bool ImageViewportScanState::pendingFinalScanStart() const { return m_pendingFinalScanStart; }
}
