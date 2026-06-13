// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageShortcutScope {
        HelpShortcutScope = 0,
        ViewerShortcutScope = 1,
        ReadyShortcutScope = 2,
        ReadyViewerShortcutScope = 3,
        ImageSelectionShortcutScope = 4,
        ImageSelectionViewerShortcutScope = 5,
        PageShortcutScope = 6,
        PageViewerShortcutScope = 7,
        RightToLeftReadingShortcutScope = 8,
        RightToLeftReadingViewerShortcutScope = 9,
        RotateShortcutScope = 10,
        RotateViewerShortcutScope = 11,
        PannableShortcutScope = 12,
        PannableViewerShortcutScope = 13,
        ContainerShortcutScope = 14,
        ContainerViewerShortcutScope = 15,
        MediaStartEndViewerShortcutScope = 16,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageActionAvailabilityInput {
        image_ready: bool,
        file_deletion_in_progress: bool,
        help_dialog_open: bool,
        text_input_focused: bool,
        image_pannable: bool,
        container_navigation_available: bool,
        two_page_mode_enabled: bool,
        two_page_mode_available: bool,
        right_to_left_reading_enabled: bool,
        right_to_left_reading_available: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageActionAvailabilityProjection {
        can_use_ready_actions: bool,
        can_use_rotate_actions: bool,
        can_use_two_page_mode_actions: bool,
        can_use_right_to_left_reading_actions: bool,
        right_to_left_reading_active: bool,
        two_page_mode_active: bool,
        help_shortcuts_enabled: bool,
        viewer_shortcuts_enabled: bool,
        ready_shortcuts_enabled: bool,
        ready_viewer_shortcuts_enabled: bool,
        two_page_viewer_shortcuts_enabled: bool,
        right_to_left_reading_shortcuts_enabled: bool,
        right_to_left_reading_viewer_shortcuts_enabled: bool,
        rotate_shortcuts_enabled: bool,
        rotate_viewer_shortcuts_enabled: bool,
        pannable_shortcuts_enabled: bool,
        pannable_viewer_shortcuts_enabled: bool,
        container_shortcuts_enabled: bool,
        container_viewer_shortcuts_enabled: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoShortcutAvailabilityInput {
        help_shortcuts_enabled: bool,
        viewer_shortcuts_enabled: bool,
        file_deletion_in_progress: bool,
        media_navigation_active: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustMediaHorizontalArrowShortcutInput {
        video_mode: bool,
        image_ready_viewer_shortcuts_enabled: bool,
        video_viewer_shortcuts_enabled: bool,
        video_media_navigation_active: bool,
        video_file_deletion_in_progress: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageActionAvailabilityProjection"]
        fn rust_image_action_availability_projection(
            input: RustImageActionAvailabilityInput,
        ) -> RustImageActionAvailabilityProjection;

        #[cxx_name = "rustImageActionAvailabilityShortcutsEnabledForScope"]
        fn rust_image_action_availability_shortcuts_enabled_for_scope(
            projection: RustImageActionAvailabilityProjection,
            scope: RustImageShortcutScope,
        ) -> bool;

        #[cxx_name = "rustActiveMediaShortcutsEnabledForScope"]
        fn rust_active_media_shortcuts_enabled_for_scope(
            projection: RustImageActionAvailabilityProjection,
            scope: RustImageShortcutScope,
            video_mode: bool,
            active_navigation_actions_available: bool,
            video_file_deletion_in_progress: bool,
        ) -> bool;

        #[cxx_name = "rustVideoShortcutsEnabledForScope"]
        fn rust_video_shortcuts_enabled_for_scope(
            input: RustVideoShortcutAvailabilityInput,
            scope: RustImageShortcutScope,
        ) -> bool;

        #[cxx_name = "rustMediaHorizontalArrowShortcutsEnabled"]
        fn rust_media_horizontal_arrow_shortcuts_enabled(
            input: RustMediaHorizontalArrowShortcutInput,
        ) -> bool;
    }
}

use ffi::{
    RustImageActionAvailabilityInput, RustImageActionAvailabilityProjection,
    RustImageShortcutScope, RustMediaHorizontalArrowShortcutInput,
    RustVideoShortcutAvailabilityInput,
};

fn rust_image_action_availability_projection(
    input: RustImageActionAvailabilityInput,
) -> RustImageActionAvailabilityProjection {
    let can_use_ready_actions =
        input.image_ready && !input.file_deletion_in_progress && !input.help_dialog_open;
    let right_to_left_reading_active =
        input.right_to_left_reading_enabled && input.right_to_left_reading_available;
    let two_page_mode_active = input.two_page_mode_enabled && input.two_page_mode_available;
    let help_shortcuts_enabled = !input.help_dialog_open;
    let viewer_shortcuts_enabled = !input.text_input_focused && help_shortcuts_enabled;
    let ready_shortcuts_enabled =
        input.image_ready && !input.file_deletion_in_progress && help_shortcuts_enabled;
    let ready_viewer_shortcuts_enabled =
        input.image_ready && !input.file_deletion_in_progress && viewer_shortcuts_enabled;

    RustImageActionAvailabilityProjection {
        can_use_ready_actions,
        can_use_rotate_actions: can_use_ready_actions && !two_page_mode_active,
        can_use_two_page_mode_actions: can_use_ready_actions && input.two_page_mode_available,
        can_use_right_to_left_reading_actions: can_use_ready_actions
            && input.right_to_left_reading_available,
        right_to_left_reading_active,
        two_page_mode_active,
        help_shortcuts_enabled,
        viewer_shortcuts_enabled,
        ready_shortcuts_enabled,
        ready_viewer_shortcuts_enabled,
        two_page_viewer_shortcuts_enabled: ready_viewer_shortcuts_enabled && two_page_mode_active,
        right_to_left_reading_shortcuts_enabled: ready_shortcuts_enabled
            && input.right_to_left_reading_available,
        right_to_left_reading_viewer_shortcuts_enabled: ready_viewer_shortcuts_enabled
            && input.right_to_left_reading_available,
        rotate_shortcuts_enabled: ready_shortcuts_enabled && !two_page_mode_active,
        rotate_viewer_shortcuts_enabled: ready_viewer_shortcuts_enabled && !two_page_mode_active,
        pannable_shortcuts_enabled: input.image_pannable
            && !input.file_deletion_in_progress
            && help_shortcuts_enabled,
        pannable_viewer_shortcuts_enabled: input.image_pannable
            && !input.file_deletion_in_progress
            && viewer_shortcuts_enabled,
        container_shortcuts_enabled: input.container_navigation_available
            && !input.file_deletion_in_progress
            && help_shortcuts_enabled,
        container_viewer_shortcuts_enabled: input.container_navigation_available
            && !input.file_deletion_in_progress
            && viewer_shortcuts_enabled,
    }
}

fn rust_image_action_availability_shortcuts_enabled_for_scope(
    projection: RustImageActionAvailabilityProjection,
    scope: RustImageShortcutScope,
) -> bool {
    image_shortcuts_enabled_for_scope(projection, scope)
}

fn rust_active_media_shortcuts_enabled_for_scope(
    projection: RustImageActionAvailabilityProjection,
    scope: RustImageShortcutScope,
    video_mode: bool,
    active_navigation_actions_available: bool,
    video_file_deletion_in_progress: bool,
) -> bool {
    active_media_shortcuts_enabled_for_scope(
        projection,
        scope,
        video_mode,
        active_navigation_actions_available,
        video_file_deletion_in_progress,
    )
}

fn image_shortcuts_enabled_for_scope(
    projection: RustImageActionAvailabilityProjection,
    scope: RustImageShortcutScope,
) -> bool {
    match scope {
        RustImageShortcutScope::HelpShortcutScope => projection.help_shortcuts_enabled,
        RustImageShortcutScope::ViewerShortcutScope => projection.viewer_shortcuts_enabled,
        RustImageShortcutScope::ReadyShortcutScope => projection.ready_shortcuts_enabled,
        RustImageShortcutScope::ReadyViewerShortcutScope => {
            projection.ready_viewer_shortcuts_enabled
        }
        RustImageShortcutScope::ImageSelectionShortcutScope
        | RustImageShortcutScope::ImageSelectionViewerShortcutScope
        | RustImageShortcutScope::PageShortcutScope
        | RustImageShortcutScope::PageViewerShortcutScope => false,
        RustImageShortcutScope::RightToLeftReadingShortcutScope => {
            projection.right_to_left_reading_shortcuts_enabled
        }
        RustImageShortcutScope::RightToLeftReadingViewerShortcutScope => {
            projection.right_to_left_reading_viewer_shortcuts_enabled
        }
        RustImageShortcutScope::RotateShortcutScope => projection.rotate_shortcuts_enabled,
        RustImageShortcutScope::RotateViewerShortcutScope => {
            projection.rotate_viewer_shortcuts_enabled
        }
        RustImageShortcutScope::PannableShortcutScope => projection.pannable_shortcuts_enabled,
        RustImageShortcutScope::PannableViewerShortcutScope
        | RustImageShortcutScope::MediaStartEndViewerShortcutScope => {
            projection.pannable_viewer_shortcuts_enabled
        }
        RustImageShortcutScope::ContainerShortcutScope => projection.container_shortcuts_enabled,
        RustImageShortcutScope::ContainerViewerShortcutScope => {
            projection.container_viewer_shortcuts_enabled
        }
        _ => false,
    }
}

fn active_media_shortcuts_enabled_for_scope(
    projection: RustImageActionAvailabilityProjection,
    scope: RustImageShortcutScope,
    video_mode: bool,
    active_navigation_actions_available: bool,
    video_file_deletion_in_progress: bool,
) -> bool {
    match scope {
        RustImageShortcutScope::ImageSelectionShortcutScope
        | RustImageShortcutScope::PageShortcutScope => active_navigation_actions_available,
        RustImageShortcutScope::ImageSelectionViewerShortcutScope
        | RustImageShortcutScope::PageViewerShortcutScope => {
            active_navigation_actions_available && projection.viewer_shortcuts_enabled
        }
        _ if video_mode => rust_video_shortcuts_enabled_for_scope(
            RustVideoShortcutAvailabilityInput {
                help_shortcuts_enabled: projection.help_shortcuts_enabled,
                viewer_shortcuts_enabled: projection.viewer_shortcuts_enabled,
                file_deletion_in_progress: video_file_deletion_in_progress,
                media_navigation_active: active_navigation_actions_available,
            },
            scope,
        ),
        _ => image_shortcuts_enabled_for_scope(projection, scope),
    }
}

fn rust_video_shortcuts_enabled_for_scope(
    input: RustVideoShortcutAvailabilityInput,
    scope: RustImageShortcutScope,
) -> bool {
    let ready_shortcuts_enabled = input.help_shortcuts_enabled && !input.file_deletion_in_progress;
    let ready_viewer_shortcuts_enabled =
        input.viewer_shortcuts_enabled && !input.file_deletion_in_progress;
    let media_shortcuts_enabled = input.media_navigation_active && ready_shortcuts_enabled;
    let media_viewer_shortcuts_enabled =
        input.media_navigation_active && ready_viewer_shortcuts_enabled;

    match scope {
        RustImageShortcutScope::HelpShortcutScope => input.help_shortcuts_enabled,
        RustImageShortcutScope::ViewerShortcutScope => input.viewer_shortcuts_enabled,
        RustImageShortcutScope::ReadyShortcutScope
        | RustImageShortcutScope::RotateShortcutScope
        | RustImageShortcutScope::PannableShortcutScope
        | RustImageShortcutScope::ContainerShortcutScope
        | RustImageShortcutScope::RightToLeftReadingShortcutScope => ready_shortcuts_enabled,
        RustImageShortcutScope::ReadyViewerShortcutScope
        | RustImageShortcutScope::RotateViewerShortcutScope
        | RustImageShortcutScope::PannableViewerShortcutScope
        | RustImageShortcutScope::MediaStartEndViewerShortcutScope
        | RustImageShortcutScope::ContainerViewerShortcutScope
        | RustImageShortcutScope::RightToLeftReadingViewerShortcutScope => {
            ready_viewer_shortcuts_enabled
        }
        RustImageShortcutScope::ImageSelectionShortcutScope
        | RustImageShortcutScope::PageShortcutScope => media_shortcuts_enabled,
        RustImageShortcutScope::ImageSelectionViewerShortcutScope
        | RustImageShortcutScope::PageViewerShortcutScope => media_viewer_shortcuts_enabled,
        _ => false,
    }
}

fn rust_media_horizontal_arrow_shortcuts_enabled(
    input: RustMediaHorizontalArrowShortcutInput,
) -> bool {
    if !input.video_mode {
        return input.image_ready_viewer_shortcuts_enabled;
    }

    input.video_media_navigation_active
        && !input.video_file_deletion_in_progress
        && input.video_viewer_shortcuts_enabled
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn derives_ready_and_mode_availability_from_snapshot() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.two_page_mode_available = true;
        input.right_to_left_reading_available = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(projection.can_use_ready_actions);
        assert!(projection.can_use_rotate_actions);
        assert!(projection.can_use_two_page_mode_actions);
        assert!(projection.can_use_right_to_left_reading_actions);
        assert!(!projection.two_page_mode_active);
        assert!(!projection.right_to_left_reading_active);

        input.two_page_mode_enabled = true;
        input.right_to_left_reading_enabled = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(projection.two_page_mode_active);
        assert!(projection.right_to_left_reading_active);
        assert!(!projection.can_use_rotate_actions);
    }

    #[test]
    fn derives_shortcut_gates_from_snapshot() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_pannable = true;
        input.container_navigation_available = true;
        input.two_page_mode_enabled = true;
        input.two_page_mode_available = true;
        input.right_to_left_reading_available = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(projection.help_shortcuts_enabled);
        assert!(projection.viewer_shortcuts_enabled);
        assert!(projection.ready_shortcuts_enabled);
        assert!(projection.ready_viewer_shortcuts_enabled);
        assert!(projection.two_page_viewer_shortcuts_enabled);
        assert!(projection.right_to_left_reading_shortcuts_enabled);
        assert!(projection.right_to_left_reading_viewer_shortcuts_enabled);
        assert!(projection.pannable_shortcuts_enabled);
        assert!(projection.pannable_viewer_shortcuts_enabled);
        assert!(projection.container_shortcuts_enabled);
        assert!(projection.container_viewer_shortcuts_enabled);
        assert!(!projection.rotate_shortcuts_enabled);
        assert!(!projection.rotate_viewer_shortcuts_enabled);

        input.text_input_focused = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(!projection.viewer_shortcuts_enabled);
        assert!(projection.ready_shortcuts_enabled);
        assert!(!projection.ready_viewer_shortcuts_enabled);
        assert!(!projection.pannable_viewer_shortcuts_enabled);
        assert!(!projection.container_viewer_shortcuts_enabled);

        input.file_deletion_in_progress = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(!projection.ready_shortcuts_enabled);
        assert!(!projection.pannable_shortcuts_enabled);
        assert!(!projection.container_shortcuts_enabled);
    }

    #[test]
    fn help_dialog_blocks_actions_and_shortcuts_consistently() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_pannable = true;
        input.container_navigation_available = true;
        input.two_page_mode_available = true;
        input.right_to_left_reading_available = true;
        input.help_dialog_open = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(!projection.can_use_ready_actions);
        assert!(!projection.can_use_two_page_mode_actions);
        assert!(!projection.can_use_right_to_left_reading_actions);
        assert!(!projection.help_shortcuts_enabled);
        assert!(!projection.viewer_shortcuts_enabled);
        assert!(!projection.ready_shortcuts_enabled);
        assert!(!projection.pannable_shortcuts_enabled);
        assert!(!projection.container_shortcuts_enabled);
    }

    #[test]
    fn video_shortcut_scopes_use_viewer_deletion_and_navigation_gates() {
        let mut input = video_shortcut_availability_input();
        input.help_shortcuts_enabled = true;
        input.viewer_shortcuts_enabled = true;
        input.media_navigation_active = true;

        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::HelpShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ViewerShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ReadyShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ReadyViewerShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ImageSelectionShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ImageSelectionViewerShortcutScope
        ));
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope
        ));

        input.viewer_shortcuts_enabled = false;
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ReadyShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ReadyViewerShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ImageSelectionViewerShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope
        ));

        input.viewer_shortcuts_enabled = true;
        input.media_navigation_active = false;
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ImageSelectionShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::PageViewerShortcutScope
        ));

        input.file_deletion_in_progress = true;
        assert!(rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::HelpShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ReadyShortcutScope
        ));
        assert!(!rust_video_shortcuts_enabled_for_scope(
            input,
            RustImageShortcutScope::ContainerViewerShortcutScope
        ));
    }

    #[test]
    fn image_shortcut_scope_lookup_uses_projection_fields() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_pannable = true;
        input.container_navigation_available = true;
        input.right_to_left_reading_available = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::HelpShortcutScope
        ));
        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ReadyViewerShortcutScope
        ));
        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::RightToLeftReadingShortcutScope
        ));
        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::PannableViewerShortcutScope
        ));
        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope
        ));
        assert!(image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ContainerShortcutScope
        ));
        assert!(!image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ImageSelectionShortcutScope
        ));
        assert!(!image_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::PageViewerShortcutScope
        ));
    }

    #[test]
    fn active_media_shortcuts_own_navigation_scopes_for_image_and_video() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_pannable = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ImageSelectionShortcutScope,
            false,
            true,
            false
        ));
        assert!(active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::PageViewerShortcutScope,
            true,
            true,
            false
        ));
        assert!(!active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::PageViewerShortcutScope,
            true,
            false,
            false
        ));
        assert!(active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope,
            false,
            true,
            false
        ));
        assert!(active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope,
            true,
            false,
            false
        ));
        assert!(!active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::MediaStartEndViewerShortcutScope,
            true,
            true,
            true
        ));
    }

    #[test]
    fn active_media_shortcuts_use_video_gates_outside_navigation_scopes() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ReadyViewerShortcutScope,
            true,
            true,
            false
        ));
        assert!(!active_media_shortcuts_enabled_for_scope(
            projection,
            RustImageShortcutScope::ReadyViewerShortcutScope,
            true,
            true,
            true
        ));
    }

    #[test]
    fn horizontal_arrow_shortcut_policy_uses_active_media_mode() {
        let mut input = media_horizontal_arrow_shortcut_input();
        input.video_viewer_shortcuts_enabled = true;
        input.video_media_navigation_active = true;

        input.video_mode = false;
        input.image_ready_viewer_shortcuts_enabled = true;
        assert!(rust_media_horizontal_arrow_shortcuts_enabled(input));

        input.image_ready_viewer_shortcuts_enabled = false;
        assert!(!rust_media_horizontal_arrow_shortcuts_enabled(input));

        input.video_mode = true;
        assert!(rust_media_horizontal_arrow_shortcuts_enabled(input));

        input.video_viewer_shortcuts_enabled = false;
        assert!(!rust_media_horizontal_arrow_shortcuts_enabled(input));

        input.video_viewer_shortcuts_enabled = true;
        input.video_file_deletion_in_progress = true;
        assert!(!rust_media_horizontal_arrow_shortcuts_enabled(input));

        input.video_file_deletion_in_progress = false;
        input.video_media_navigation_active = false;
        assert!(!rust_media_horizontal_arrow_shortcuts_enabled(input));
    }

    fn image_action_availability_input() -> RustImageActionAvailabilityInput {
        RustImageActionAvailabilityInput {
            image_ready: false,
            file_deletion_in_progress: false,
            help_dialog_open: false,
            text_input_focused: false,
            image_pannable: false,
            container_navigation_available: false,
            two_page_mode_enabled: false,
            two_page_mode_available: false,
            right_to_left_reading_enabled: false,
            right_to_left_reading_available: false,
        }
    }

    fn video_shortcut_availability_input() -> RustVideoShortcutAvailabilityInput {
        RustVideoShortcutAvailabilityInput {
            help_shortcuts_enabled: false,
            viewer_shortcuts_enabled: false,
            file_deletion_in_progress: false,
            media_navigation_active: false,
        }
    }

    fn media_horizontal_arrow_shortcut_input() -> RustMediaHorizontalArrowShortcutInput {
        RustMediaHorizontalArrowShortcutInput {
            video_mode: false,
            image_ready_viewer_shortcuts_enabled: false,
            video_viewer_shortcuts_enabled: false,
            video_media_navigation_active: false,
            video_file_deletion_in_progress: false,
        }
    }
}
