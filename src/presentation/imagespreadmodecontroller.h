// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADMODECONTROLLER_H
#define KIRIVIEW_IMAGESPREADMODECONTROLLER_H

#include "presentation/imagepresentationstate.h"
#include "presentation/imagespreadmodepolicy.h"

namespace KiriView {
class ImageSpreadModeController final
{
public:
    bool twoPageModeEnabled() const;
    ImageSpreadTwoPageModeChange setTwoPageModeEnabled(bool enabled, bool secondaryPageVisible);
    bool twoPageModeAvailable(const ImageSpreadReadingAvailability &availability) const;
    bool twoPageModeActive(const ImageSpreadReadingAvailability &availability) const;

    bool rightToLeftReadingEnabled() const;
    bool setRightToLeftReadingEnabled(bool enabled);
    bool rightToLeftReadingAvailable(const ImageSpreadReadingAvailability &availability) const;
    bool rightToLeftReadingActive(const ImageSpreadReadingAvailability &availability) const;
    void resetRightToLeftReading();

    bool spreadTransitionInProgress() const;
    ImagePresentationTransitionState presentationTransitionState() const;
    bool beginSpreadTransition();
    bool finishSpreadTransition();

private:
    bool m_twoPageModeEnabled = false;
    bool m_rightToLeftReadingEnabled = false;
    bool m_spreadTransitionInProgress = false;
};
}

#endif
