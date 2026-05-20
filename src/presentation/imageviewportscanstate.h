// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTSCANSTATE_H
#define KIRIVIEW_IMAGEVIEWPORTSCANSTATE_H

namespace KiriView {
enum class ImageViewportScanStart {
    Initial,
    Final,
};

class ImageViewportScanState final
{
public:
    void requestNextDisplayedImageFinalScanStart();
    ImageViewportScanStart beginDisplayedImage();
    void cancelPendingDisplayedImageStart();
    ImageViewportScanStart displayedImageScanStart() const;
    bool pendingFinalScanStart() const;

private:
    bool m_pendingFinalScanStart = false;
    ImageViewportScanStart m_displayedImageScanStart = ImageViewportScanStart::Initial;
};
}

#endif
