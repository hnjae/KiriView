// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilitypolicy.h"

#include "kiriview/src/policy/imageactionavailability.cxx.h"

namespace {
KiriView::RustImageActionAvailabilityInput rustImageActionAvailabilityInput(
    const ImageActionAvailabilityInput &input)
{
    return KiriView::RustImageActionAvailabilityInput {
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

KiriView::RustImageShortcutScope rustImageShortcutScope(
    KiriView::ApplicationActions::ImageShortcutScope scope)
{
    using Scope = KiriView::ApplicationActions::ImageShortcutScope;

    switch (scope) {
    case Scope::HelpShortcutScope:
        return KiriView::RustImageShortcutScope::HelpShortcutScope;
    case Scope::ViewerShortcutScope:
        return KiriView::RustImageShortcutScope::ViewerShortcutScope;
    case Scope::ReadyShortcutScope:
        return KiriView::RustImageShortcutScope::ReadyShortcutScope;
    case Scope::ReadyViewerShortcutScope:
        return KiriView::RustImageShortcutScope::ReadyViewerShortcutScope;
    case Scope::ImageSelectionShortcutScope:
        return KiriView::RustImageShortcutScope::ImageSelectionShortcutScope;
    case Scope::ImageSelectionViewerShortcutScope:
        return KiriView::RustImageShortcutScope::ImageSelectionViewerShortcutScope;
    case Scope::PageShortcutScope:
        return KiriView::RustImageShortcutScope::PageShortcutScope;
    case Scope::PageViewerShortcutScope:
        return KiriView::RustImageShortcutScope::PageViewerShortcutScope;
    case Scope::RightToLeftReadingShortcutScope:
        return KiriView::RustImageShortcutScope::RightToLeftReadingShortcutScope;
    case Scope::RightToLeftReadingViewerShortcutScope:
        return KiriView::RustImageShortcutScope::RightToLeftReadingViewerShortcutScope;
    case Scope::RotateShortcutScope:
        return KiriView::RustImageShortcutScope::RotateShortcutScope;
    case Scope::RotateViewerShortcutScope:
        return KiriView::RustImageShortcutScope::RotateViewerShortcutScope;
    case Scope::PannableShortcutScope:
        return KiriView::RustImageShortcutScope::PannableShortcutScope;
    case Scope::PannableViewerShortcutScope:
        return KiriView::RustImageShortcutScope::PannableViewerShortcutScope;
    case Scope::ContainerShortcutScope:
        return KiriView::RustImageShortcutScope::ContainerShortcutScope;
    case Scope::ContainerViewerShortcutScope:
        return KiriView::RustImageShortcutScope::ContainerViewerShortcutScope;
    }

    return KiriView::RustImageShortcutScope::HelpShortcutScope;
}

KiriView::RustVideoShortcutAvailabilityInput rustVideoShortcutAvailabilityInput(
    const KiriView::ApplicationActions::VideoShortcutAvailabilityInput &input)
{
    return KiriView::RustVideoShortcutAvailabilityInput {
        input.helpShortcutsEnabled,
        input.viewerShortcutsEnabled,
        input.fileDeletionInProgress,
        input.mediaNavigationActive,
    };
}

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const KiriView::RustImageActionAvailabilityProjection &projection)
{
    return ImageActionAvailabilityProjection {
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

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput &input)
{
    return imageActionAvailabilityProjection(
        KiriView::rustImageActionAvailabilityProjection(rustImageActionAvailabilityInput(input)));
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
        rustVideoShortcutAvailabilityInput(input), rustImageShortcutScope(scope));
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
