// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadmodecontroller.h"

namespace KiriView {
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

bool ImageSpreadModeController::twoPageModeAvailable(
    const ImageSpreadReadingAvailability &availability) const
{
    return imageSpreadReadingControlsAvailable(availability);
}

bool ImageSpreadModeController::twoPageModeActive(
    const ImageSpreadReadingAvailability &availability) const
{
    return m_twoPageModeEnabled && twoPageModeAvailable(availability);
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

bool ImageSpreadModeController::rightToLeftReadingAvailable(
    const ImageSpreadReadingAvailability &availability) const
{
    return imageSpreadReadingControlsAvailable(availability);
}

bool ImageSpreadModeController::rightToLeftReadingActive(
    const ImageSpreadReadingAvailability &availability) const
{
    return m_rightToLeftReadingEnabled && rightToLeftReadingAvailable(availability);
}

void ImageSpreadModeController::resetRightToLeftReading() { m_rightToLeftReadingEnabled = false; }

bool ImageSpreadModeController::spreadTransitionInProgress() const
{
    return m_spreadTransitionInProgress;
}

ImagePresentationTransitionState ImageSpreadModeController::presentationTransitionState() const
{
    return m_spreadTransitionInProgress ? ImagePresentationTransitionState::TransitioningPlaceholder
                                        : ImagePresentationTransitionState::CommittedActive;
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
}
