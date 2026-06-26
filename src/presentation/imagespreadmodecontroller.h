// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADMODECONTROLLER_H
#define KIRIVIEW_IMAGESPREADMODECONTROLLER_H

#include "presentation/imagespreadmodepolicy.h"

namespace kiriview {
class ImageSpreadModeController final
{
public:
    bool twoPageModeEnabled() const;
    ImageSpreadTwoPageModeChange setTwoPageModeEnabled(bool enabled, bool secondaryPageVisible);
    bool twoPageModeAvailable(ImageSpreadReadingAvailability availability) const;
    bool twoPageModeActive(ImageSpreadReadingAvailability availability) const;

    bool rightToLeftReadingEnabled() const;
    bool setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable(ImageSpreadReadingAvailability availability) const;
    bool rightToLeftReadingActive(ImageSpreadReadingAvailability availability) const;
    void resetRightToLeftReading();

private:
    bool m_twoPageModeEnabled = false;
    bool m_rightToLeftReadingEnabled = false;
};
}

#endif
