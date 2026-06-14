// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilitypolicy.h"

#include "bridge/imageactionavailabilityconversion.h"
#include "kiriview/src/policy/imageactionavailability.cxx.h"

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput &input)
{
    return kiriview::Bridge::imageActionAvailabilityProjectionFromRust(
        kiriview::rustImageActionAvailabilityProjection(
            kiriview::Bridge::rustImageActionAvailabilityInput(input)));
}

bool imageActionAvailabilityShortcutsEnabledForScope(
    const ImageActionAvailabilityProjection &projection,
    kiriview::ApplicationActions::ImageShortcutScope scope)
{
    if (!kiriview::ApplicationActions::imageShortcutScopeKnown(scope)) {
        return false;
    }

    return kiriview::rustImageActionAvailabilityShortcutsEnabledForScope(
        kiriview::Bridge::rustImageActionAvailabilityProjection(projection),
        kiriview::Bridge::rustImageShortcutScope(scope));
}

bool activeMediaShortcutsEnabledForScope(const ActiveMediaShortcutAvailabilityInput &input,
    kiriview::ApplicationActions::ImageShortcutScope scope)
{
    if (!kiriview::ApplicationActions::imageShortcutScopeKnown(scope)) {
        return false;
    }

    return kiriview::rustActiveMediaShortcutsEnabledForScope(
        kiriview::Bridge::rustImageActionAvailabilityProjection(input.imageProjection),
        kiriview::Bridge::rustImageShortcutScope(scope), input.videoMode,
        input.activeNavigationActionsAvailable, input.videoFileDeletionInProgress);
}

namespace kiriview::ApplicationActions {
bool videoShortcutsEnabledForScope(
    const VideoShortcutAvailabilityInput &input, ImageShortcutScope scope)
{
    if (!imageShortcutScopeKnown(scope)) {
        return false;
    }

    return kiriview::rustVideoShortcutsEnabledForScope(
        kiriview::Bridge::rustVideoShortcutAvailabilityInput(input),
        kiriview::Bridge::rustImageShortcutScope(scope));
}

bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
    const VideoShortcutAvailabilityInput &videoInput)
{
    return kiriview::rustMediaHorizontalArrowShortcutsEnabled(
        kiriview::RustMediaHorizontalArrowShortcutInput {
            videoMode,
            imageReadyViewerShortcutsEnabled,
            videoInput.viewerShortcutsEnabled,
            videoInput.directMediaNavigationActive,
            videoInput.fileDeletionInProgress,
        });
}
}
