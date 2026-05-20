// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageActionAvailabilityInput {
        image_ready: bool,
        image_count: i32,
        current_page_number: i32,
        current_last_page_number: i32,
        file_deletion_in_progress: bool,
        help_dialog_open: bool,
        text_input_focused: bool,
        image_pannable: bool,
        image_horizontally_pannable: bool,
        container_navigation_available: bool,
        two_page_mode_enabled: bool,
        two_page_mode_available: bool,
        right_to_left_reading_enabled: bool,
        right_to_left_reading_available: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageActionAvailabilityProjection {
        can_open_next_image: bool,
        can_open_previous_image: bool,
        at_known_first_image: bool,
        at_known_last_image: bool,
        can_use_page_actions: bool,
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
        image_selection_shortcuts_enabled: bool,
        image_selection_viewer_shortcuts_enabled: bool,
        page_shortcuts_enabled: bool,
        page_viewer_shortcuts_enabled: bool,
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

    extern "Rust" {
        #[cxx_name = "rustImageActionAvailabilityProjection"]
        fn rust_image_action_availability_projection(
            input: RustImageActionAvailabilityInput,
        ) -> RustImageActionAvailabilityProjection;
    }
}

use ffi::{RustImageActionAvailabilityInput, RustImageActionAvailabilityProjection};

fn rust_image_action_availability_projection(
    input: RustImageActionAvailabilityInput,
) -> RustImageActionAvailabilityProjection {
    let can_use_page_actions = input.image_count > 0
        && input.current_page_number > 0
        && !input.file_deletion_in_progress
        && !input.help_dialog_open;
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
    let image_selection_shortcuts_enabled = input.image_count > 0
        && input.current_page_number > 0
        && !input.file_deletion_in_progress
        && help_shortcuts_enabled;
    let image_selection_viewer_shortcuts_enabled = input.image_count > 0
        && input.current_page_number > 0
        && !input.file_deletion_in_progress
        && viewer_shortcuts_enabled;

    RustImageActionAvailabilityProjection {
        can_open_next_image: input.current_page_number > 0
            && input.current_last_page_number < input.image_count,
        can_open_previous_image: input.current_page_number > 1,
        at_known_first_image: input.image_count > 0 && input.current_page_number == 1,
        at_known_last_image: input.image_count > 0
            && input.current_page_number > 0
            && input.current_last_page_number > 0
            && input.current_last_page_number >= input.image_count,
        can_use_page_actions,
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
        image_selection_shortcuts_enabled,
        image_selection_viewer_shortcuts_enabled,
        page_shortcuts_enabled: image_selection_shortcuts_enabled,
        page_viewer_shortcuts_enabled: image_selection_viewer_shortcuts_enabled,
        two_page_viewer_shortcuts_enabled: image_selection_viewer_shortcuts_enabled
            && two_page_mode_active,
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn derives_navigation_and_mode_availability_from_snapshot() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_count = 5;
        input.current_page_number = 1;
        input.current_last_page_number = 2;
        input.two_page_mode_available = true;
        input.right_to_left_reading_available = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(projection.can_use_page_actions);
        assert!(!projection.can_open_previous_image);
        assert!(projection.can_open_next_image);
        assert!(projection.at_known_first_image);
        assert!(!projection.at_known_last_image);
        assert!(projection.can_use_ready_actions);
        assert!(projection.can_use_rotate_actions);
        assert!(projection.can_use_two_page_mode_actions);
        assert!(projection.can_use_right_to_left_reading_actions);
        assert!(!projection.two_page_mode_active);
        assert!(!projection.right_to_left_reading_active);

        input.current_page_number = 5;
        input.current_last_page_number = 5;
        input.two_page_mode_enabled = true;
        input.right_to_left_reading_enabled = true;
        let projection = rust_image_action_availability_projection(input);

        assert!(projection.can_open_previous_image);
        assert!(!projection.can_open_next_image);
        assert!(!projection.at_known_first_image);
        assert!(projection.at_known_last_image);
        assert!(projection.two_page_mode_active);
        assert!(projection.right_to_left_reading_active);
        assert!(!projection.can_use_rotate_actions);
    }

    #[test]
    fn derives_shortcut_gates_from_snapshot() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_count = 3;
        input.current_page_number = 2;
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
        assert!(projection.image_selection_shortcuts_enabled);
        assert!(projection.image_selection_viewer_shortcuts_enabled);
        assert!(projection.page_shortcuts_enabled);
        assert!(projection.page_viewer_shortcuts_enabled);
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
        assert!(!projection.image_selection_shortcuts_enabled);
        assert!(!projection.pannable_shortcuts_enabled);
        assert!(!projection.container_shortcuts_enabled);
    }

    #[test]
    fn help_dialog_blocks_actions_and_shortcuts_consistently() {
        let mut input = image_action_availability_input();
        input.image_ready = true;
        input.image_count = 3;
        input.current_page_number = 2;
        input.image_pannable = true;
        input.container_navigation_available = true;
        input.two_page_mode_available = true;
        input.right_to_left_reading_available = true;
        input.help_dialog_open = true;

        let projection = rust_image_action_availability_projection(input);

        assert!(!projection.can_use_page_actions);
        assert!(!projection.can_use_ready_actions);
        assert!(!projection.can_use_two_page_mode_actions);
        assert!(!projection.can_use_right_to_left_reading_actions);
        assert!(!projection.help_shortcuts_enabled);
        assert!(!projection.viewer_shortcuts_enabled);
        assert!(!projection.ready_shortcuts_enabled);
        assert!(!projection.image_selection_shortcuts_enabled);
        assert!(!projection.pannable_shortcuts_enabled);
        assert!(!projection.container_shortcuts_enabled);
    }

    fn image_action_availability_input() -> RustImageActionAvailabilityInput {
        RustImageActionAvailabilityInput {
            image_ready: false,
            image_count: 0,
            current_page_number: 0,
            current_last_page_number: 0,
            file_deletion_in_progress: false,
            help_dialog_open: false,
            text_input_focused: false,
            image_pannable: false,
            image_horizontally_pannable: false,
            container_navigation_available: false,
            two_page_mode_enabled: false,
            two_page_mode_available: false,
            right_to_left_reading_enabled: false,
            right_to_left_reading_available: false,
        }
    }
}
