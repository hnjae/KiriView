// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilitypolicy.h"

#include "bridge/imageactionavailabilityconversion.h"
#include "kiriview/src/policy/imageactionavailability.cxx.h"

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
    using KiriView::ApplicationActions::ImageShortcutScope;

    switch (scope) {
    case ImageShortcutScope::HelpShortcutScope:
        return projection.helpShortcutsEnabled;
    case ImageShortcutScope::ViewerShortcutScope:
        return projection.viewerShortcutsEnabled;
    case ImageShortcutScope::ReadyShortcutScope:
        return projection.readyShortcutsEnabled;
    case ImageShortcutScope::ReadyViewerShortcutScope:
        return projection.readyViewerShortcutsEnabled;
    case ImageShortcutScope::ImageSelectionShortcutScope:
    case ImageShortcutScope::ImageSelectionViewerShortcutScope:
    case ImageShortcutScope::PageShortcutScope:
    case ImageShortcutScope::PageViewerShortcutScope:
        return false;
    case ImageShortcutScope::RightToLeftReadingShortcutScope:
        return projection.rightToLeftReadingShortcutsEnabled;
    case ImageShortcutScope::RightToLeftReadingViewerShortcutScope:
        return projection.rightToLeftReadingViewerShortcutsEnabled;
    case ImageShortcutScope::RotateShortcutScope:
        return projection.rotateShortcutsEnabled;
    case ImageShortcutScope::RotateViewerShortcutScope:
        return projection.rotateViewerShortcutsEnabled;
    case ImageShortcutScope::PannableShortcutScope:
        return projection.pannableShortcutsEnabled;
    case ImageShortcutScope::PannableViewerShortcutScope:
        return projection.pannableViewerShortcutsEnabled;
    case ImageShortcutScope::ContainerShortcutScope:
        return projection.containerShortcutsEnabled;
    case ImageShortcutScope::ContainerViewerShortcutScope:
        return projection.containerViewerShortcutsEnabled;
    }

    return false;
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
            videoInput.mediaNavigationActive,
            videoInput.fileDeletionInProgress,
        });
}
}
