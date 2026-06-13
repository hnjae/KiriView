// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilitypolicy.h"

#include "bridge/imageactionavailabilityconversion.h"
#include "kiriview/src/policy/imageactionavailability.cxx.h"

namespace {
bool imageShortcutScopeKnown(KiriView::ApplicationActions::ImageShortcutScope scope)
{
    using KiriView::ApplicationActions::ImageShortcutScope;

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
    return KiriView::Bridge::imageActionAvailabilityProjectionFromRust(
        KiriView::rustImageActionAvailabilityProjection(
            KiriView::Bridge::rustImageActionAvailabilityInput(input)));
}

bool imageActionAvailabilityShortcutsEnabledForScope(
    const ImageActionAvailabilityProjection &projection,
    KiriView::ApplicationActions::ImageShortcutScope scope)
{
    if (!imageShortcutScopeKnown(scope)) {
        return false;
    }

    return KiriView::rustImageActionAvailabilityShortcutsEnabledForScope(
        KiriView::Bridge::rustImageActionAvailabilityProjection(projection),
        KiriView::Bridge::rustImageShortcutScope(scope));
}

bool activeMediaShortcutsEnabledForScope(const ActiveMediaShortcutAvailabilityInput &input,
    KiriView::ApplicationActions::ImageShortcutScope scope)
{
    if (!imageShortcutScopeKnown(scope)) {
        return false;
    }

    return KiriView::rustActiveMediaShortcutsEnabledForScope(
        KiriView::Bridge::rustImageActionAvailabilityProjection(input.imageProjection),
        KiriView::Bridge::rustImageShortcutScope(scope), input.videoMode,
        input.activeNavigationActionsAvailable, input.videoFileDeletionInProgress);
}

namespace KiriView::ApplicationActions {
bool videoShortcutsEnabledForScope(
    const VideoShortcutAvailabilityInput &input, ImageShortcutScope scope)
{
    return KiriView::rustVideoShortcutsEnabledForScope(
        KiriView::Bridge::rustVideoShortcutAvailabilityInput(input),
        KiriView::Bridge::rustImageShortcutScope(scope));
}

bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
    const VideoShortcutAvailabilityInput &videoInput)
{
    return KiriView::rustMediaHorizontalArrowShortcutsEnabled(
        KiriView::RustMediaHorizontalArrowShortcutInput {
            videoMode,
            imageReadyViewerShortcutsEnabled,
            videoInput.viewerShortcutsEnabled,
            videoInput.directMediaNavigationActive,
            videoInput.fileDeletionInProgress,
        });
}
}
