// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADMODECONTROLLER_H
#define KIRIVIEW_IMAGESPREADMODECONTROLLER_H

#include "imagelocation.h"
#include "imagespreadgeometry.h"

#include <QUrl>
#include <functional>

namespace KiriView {
struct ImageSpreadModeAvailability {
    bool hasImage = false;
    DisplayedImageLocation displayedImageLocation;
};

class ImageSpreadModeController final
{
public:
    using AvailabilityProvider = std::function<ImageSpreadModeAvailability()>;

    explicit ImageSpreadModeController(AvailabilityProvider availabilityProvider);

    bool twoPageModeEnabled() const;
    ImageSpreadTwoPageModeChange setTwoPageModeEnabled(bool enabled, bool secondaryPageVisible);
    bool twoPageModeAvailable() const;
    bool twoPageModeActive() const;

    bool rightToLeftReadingEnabled() const;
    bool setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable() const;
    bool rightToLeftReadingActive() const;
    void resetRightToLeftReading();

    bool spreadTransitionInProgress() const;
    bool beginSpreadTransition();
    bool finishSpreadTransition();

private:
    ImageSpreadModeAvailability availability() const;
    bool readingControlsAvailable() const;

    AvailabilityProvider m_availabilityProvider;
    bool m_twoPageModeEnabled = false;
    bool m_rightToLeftReadingEnabled = false;
    bool m_spreadTransitionInProgress = false;
};
}

#endif
