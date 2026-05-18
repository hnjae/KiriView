// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadmodecontroller.h"

#include "imagecontainer.h"

#include <utility>

namespace KiriView {
ImageSpreadModeController::ImageSpreadModeController(AvailabilityProvider availabilityProvider)
    : m_availabilityProvider(std::move(availabilityProvider))
{
}

bool ImageSpreadModeController::twoPageModeEnabled() const { return m_twoPageModeEnabled; }

ImageSpreadTwoPageModeChange ImageSpreadModeController::setTwoPageModeEnabled(
    bool enabled, bool secondaryPageVisible)
{
    ImageSpreadTwoPageModeChange change
        = imageSpreadTwoPageModeChange(m_twoPageModeEnabled, enabled, secondaryPageVisible);
    if (change.changed) {
        m_twoPageModeEnabled = enabled;
    }

    return change;
}

bool ImageSpreadModeController::twoPageModeAvailable() const { return readingControlsAvailable(); }

bool ImageSpreadModeController::twoPageModeActive() const
{
    return m_twoPageModeEnabled && twoPageModeAvailable();
}

bool ImageSpreadModeController::rightToLeftReadingEnabled() const
{
    return m_rightToLeftReadingEnabled;
}

bool ImageSpreadModeController::setRightToLeftReadingEnabled(bool enabled)
{
    if (m_rightToLeftReadingEnabled == enabled) {
        return false;
    }

    m_rightToLeftReadingEnabled = enabled;
    return true;
}

bool ImageSpreadModeController::rightToLeftReadingAvailable() const
{
    return readingControlsAvailable();
}

bool ImageSpreadModeController::rightToLeftReadingActive() const
{
    return m_rightToLeftReadingEnabled && rightToLeftReadingAvailable();
}

void ImageSpreadModeController::resetRightToLeftReading() { m_rightToLeftReadingEnabled = false; }

bool ImageSpreadModeController::spreadTransitionInProgress() const
{
    return m_spreadTransitionInProgress;
}

bool ImageSpreadModeController::beginSpreadTransition()
{
    if (m_spreadTransitionInProgress) {
        return false;
    }

    m_spreadTransitionInProgress = true;
    return true;
}

bool ImageSpreadModeController::finishSpreadTransition()
{
    if (!m_spreadTransitionInProgress) {
        return false;
    }

    m_spreadTransitionInProgress = false;
    return true;
}

ImageSpreadModeAvailability ImageSpreadModeController::availability() const
{
    return m_availabilityProvider ? m_availabilityProvider() : ImageSpreadModeAvailability {};
}

bool ImageSpreadModeController::readingControlsAvailable() const
{
    const ImageSpreadModeAvailability currentAvailability = availability();
    const DisplayedImageLocation &location = currentAvailability.displayedImageLocation;
    return currentAvailability.hasImage && !location.imageUrl().isEmpty()
        && location.archiveDocument().isComicBook();
}
}
