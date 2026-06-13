// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imageactionavailabilityconversion.h"

namespace kiriview::Bridge {
RustImageActionAvailabilityInput rustImageActionAvailabilityInput(
    const ::ImageActionAvailabilityInput &input)
{
    return RustImageActionAvailabilityInput {
        input.imageReady,
        input.fileDeletionInProgress,
        input.helpDialogOpen,
        input.textInputFocused,
        input.imagePannable,
        input.containerNavigationAvailable,
        input.twoPageModeEnabled,
        input.twoPageModeAvailable,
        input.rightToLeftReadingEnabled,
        input.rightToLeftReadingAvailable,
    };
}

RustImageActionAvailabilityProjection rustImageActionAvailabilityProjection(
    const ::ImageActionAvailabilityProjection &projection)
{
    return RustImageActionAvailabilityProjection {
        projection.canUseReadyActions,
        projection.canUseRotateActions,
        projection.canUseTwoPageModeActions,
        projection.canUseRightToLeftReadingActions,
        projection.rightToLeftReadingActive,
        projection.twoPageModeActive,
        projection.helpShortcutsEnabled,
        projection.viewerShortcutsEnabled,
        projection.readyShortcutsEnabled,
        projection.readyViewerShortcutsEnabled,
        projection.twoPageViewerShortcutsEnabled,
        projection.rightToLeftReadingShortcutsEnabled,
        projection.rightToLeftReadingViewerShortcutsEnabled,
        projection.rotateShortcutsEnabled,
        projection.rotateViewerShortcutsEnabled,
        projection.pannableShortcutsEnabled,
        projection.pannableViewerShortcutsEnabled,
        projection.containerShortcutsEnabled,
        projection.containerViewerShortcutsEnabled,
    };
}

RustImageShortcutScope rustImageShortcutScope(ApplicationActions::ImageShortcutScope scope)
{
    using Scope = ApplicationActions::ImageShortcutScope;

    switch (scope) {
    case Scope::HelpShortcutScope:
        return RustImageShortcutScope::HelpShortcutScope;
    case Scope::ViewerShortcutScope:
        return RustImageShortcutScope::ViewerShortcutScope;
    case Scope::ReadyShortcutScope:
        return RustImageShortcutScope::ReadyShortcutScope;
    case Scope::ReadyViewerShortcutScope:
        return RustImageShortcutScope::ReadyViewerShortcutScope;
    case Scope::ImageSelectionShortcutScope:
        return RustImageShortcutScope::ImageSelectionShortcutScope;
    case Scope::ImageSelectionViewerShortcutScope:
        return RustImageShortcutScope::ImageSelectionViewerShortcutScope;
    case Scope::PageShortcutScope:
        return RustImageShortcutScope::PageShortcutScope;
    case Scope::PageViewerShortcutScope:
        return RustImageShortcutScope::PageViewerShortcutScope;
    case Scope::RightToLeftReadingShortcutScope:
        return RustImageShortcutScope::RightToLeftReadingShortcutScope;
    case Scope::RightToLeftReadingViewerShortcutScope:
        return RustImageShortcutScope::RightToLeftReadingViewerShortcutScope;
    case Scope::RotateShortcutScope:
        return RustImageShortcutScope::RotateShortcutScope;
    case Scope::RotateViewerShortcutScope:
        return RustImageShortcutScope::RotateViewerShortcutScope;
    case Scope::PannableShortcutScope:
        return RustImageShortcutScope::PannableShortcutScope;
    case Scope::PannableViewerShortcutScope:
        return RustImageShortcutScope::PannableViewerShortcutScope;
    case Scope::ContainerShortcutScope:
        return RustImageShortcutScope::ContainerShortcutScope;
    case Scope::ContainerViewerShortcutScope:
        return RustImageShortcutScope::ContainerViewerShortcutScope;
    case Scope::MediaStartEndViewerShortcutScope:
        return RustImageShortcutScope::MediaStartEndViewerShortcutScope;
    }

    return RustImageShortcutScope::HelpShortcutScope;
}

RustVideoShortcutAvailabilityInput rustVideoShortcutAvailabilityInput(
    const ApplicationActions::VideoShortcutAvailabilityInput &input)
{
    return RustVideoShortcutAvailabilityInput {
        input.helpShortcutsEnabled,
        input.viewerShortcutsEnabled,
        input.fileDeletionInProgress,
        input.directMediaNavigationActive,
    };
}

::ImageActionAvailabilityProjection imageActionAvailabilityProjectionFromRust(
    const RustImageActionAvailabilityProjection &projection)
{
    return ::ImageActionAvailabilityProjection {
        projection.can_use_ready_actions,
        projection.can_use_rotate_actions,
        projection.can_use_two_page_mode_actions,
        projection.can_use_right_to_left_reading_actions,
        projection.right_to_left_reading_active,
        projection.two_page_mode_active,
        projection.help_shortcuts_enabled,
        projection.viewer_shortcuts_enabled,
        projection.ready_shortcuts_enabled,
        projection.ready_viewer_shortcuts_enabled,
        projection.two_page_viewer_shortcuts_enabled,
        projection.right_to_left_reading_shortcuts_enabled,
        projection.right_to_left_reading_viewer_shortcuts_enabled,
        projection.rotate_shortcuts_enabled,
        projection.rotate_viewer_shortcuts_enabled,
        projection.pannable_shortcuts_enabled,
        projection.pannable_viewer_shortcuts_enabled,
        projection.container_shortcuts_enabled,
        projection.container_viewer_shortcuts_enabled,
    };
}
}
