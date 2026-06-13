// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilitypolicy.h"

#include "bridge/imageactionavailabilityconversion.h"
#include "kiriview/src/policy/imageactionavailability.cxx.h"

namespace {
bool imageShortcutScopeKnown(kiriview::ApplicationActions::ImageShortcutScope scope)
{
    using kiriview::ApplicationActions::ImageShortcutScope;

    switch (scope) {
    case ImageShortcutScope::HelpShortcutScope:
    case ImageShortcutScope::ViewerShortcutScope:
    case ImageShortcutScope::ReadyShortcutScope:
    case ImageShortcutScope::ReadyViewerShortcutScope:
    case ImageShortcutScope::ImageSelectionShortcutScope:
    case ImageShortcutScope::ImageSelectionViewerShortcutScope:
    case ImageShortcutScope::PageShortcutScope:
    case ImageShortcutScope::PageViewerShortcutScope:
    case ImageShortcutScope::RightToLeftReadingShortcutScope:
    case ImageShortcutScope::RightToLeftReadingViewerShortcutScope:
    case ImageShortcutScope::RotateShortcutScope:
    case ImageShortcutScope::RotateViewerShortcutScope:
    case ImageShortcutScope::PannableShortcutScope:
    case ImageShortcutScope::PannableViewerShortcutScope:
    case ImageShortcutScope::ContainerShortcutScope:
    case ImageShortcutScope::ContainerViewerShortcutScope:
    case ImageShortcutScope::MediaStartEndViewerShortcutScope:
        return true;
    }

    return false;
}
}

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
    if (!imageShortcutScopeKnown(scope)) {
        return false;
    }

    return kiriview::rustImageActionAvailabilityShortcutsEnabledForScope(
        kiriview::Bridge::rustImageActionAvailabilityProjection(projection),
        kiriview::Bridge::rustImageShortcutScope(scope));
}

bool activeMediaShortcutsEnabledForScope(const ActiveMediaShortcutAvailabilityInput &input,
    kiriview::ApplicationActions::ImageShortcutScope scope)
{
    if (!imageShortcutScopeKnown(scope)) {
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
