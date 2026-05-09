// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenBoolTarget {
        Unchanged = 0,
        False = 1,
        True = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenStatusTarget {
        Unchanged = 0,
        Null = 1,
        Loading = 2,
        Ready = 3,
        Error = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenErrorStringTarget {
        Unchanged = 0,
        Clear = 1,
        Provided = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenUrlTarget {
        Unchanged = 0,
        Empty = 1,
        SessionImage = 2,
        SessionContainerNavigation = 3,
        DerivedContainerNavigation = 4,
        Container = 5,
        Displayed = 6,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenDisplayedLocationTarget {
        Unchanged = 0,
        SessionLocation = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenSourceTarget {
        EmptySource = 0,
        LoadSource = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenFailureTarget {
        ContainerNavigation = 0,
        Replacement = 1,
        Initial = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenEffects {
        clear_image: bool,
        reset_zoom: bool,
        update_page_navigation: bool,
        schedule_adjacent_image_predecode: bool,
        prepare_failed_container: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenTransition {
        source_url: RustImageOpenUrlTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        loading: RustImageOpenBoolTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
        effects: RustImageOpenEffects,
    }

    extern "Rust" {
        #[cxx_name = "rustImageOpenSourceTarget"]
        fn rust_image_open_source_target(source_url_empty: bool) -> RustImageOpenSourceTarget;

        #[cxx_name = "rustImageOpenBeginSourceLoad"]
        fn rust_image_open_begin_source_load(
            has_image: bool,
            loading_container_navigation_url_empty: bool,
        ) -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishEmptySourceLoad"]
        fn rust_image_open_finish_empty_source_load() -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishSuccessfulImageLoad"]
        fn rust_image_open_finish_successful_image_load(
            request_container_navigation_url_empty: bool,
        ) -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishContainerNavigationLoadWithError"]
        fn rust_image_open_finish_container_navigation_load_with_error() -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishReplacementLoadWithError"]
        fn rust_image_open_finish_replacement_load_with_error(
            displayed_url_empty: bool,
        ) -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishInitialLoadWithError"]
        fn rust_image_open_finish_initial_load_with_error() -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishAnimationLoadWithError"]
        fn rust_image_open_finish_animation_load_with_error() -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFailureTarget"]
        fn rust_image_open_failure_target(
            container_navigation_url_empty: bool,
            has_image: bool,
        ) -> RustImageOpenFailureTarget;
    }
}

use ffi::{
    RustImageOpenBoolTarget, RustImageOpenDisplayedLocationTarget, RustImageOpenEffects,
    RustImageOpenErrorStringTarget, RustImageOpenFailureTarget, RustImageOpenSourceTarget,
    RustImageOpenStatusTarget, RustImageOpenTransition, RustImageOpenUrlTarget,
};

fn rust_image_open_source_target(source_url_empty: bool) -> RustImageOpenSourceTarget {
    if source_url_empty {
        RustImageOpenSourceTarget::EmptySource
    } else {
        RustImageOpenSourceTarget::LoadSource
    }
}

fn rust_image_open_begin_source_load(
    has_image: bool,
    loading_container_navigation_url_empty: bool,
) -> RustImageOpenTransition {
    let mut transition = empty_transition();

    if !has_image && loading_container_navigation_url_empty {
        transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    }

    transition.loading = RustImageOpenBoolTarget::True;
    if has_image {
        transition.status = RustImageOpenStatusTarget::Ready;
    } else {
        transition.effects.clear_image = true;
        transition.effects.reset_zoom = true;
        transition.status = RustImageOpenStatusTarget::Loading;
    }

    transition
}

fn rust_image_open_finish_empty_source_load() -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.effects.clear_image = true;
    transition.effects.reset_zoom = true;
    transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    transition.status = RustImageOpenStatusTarget::Null;
    transition
}

fn rust_image_open_finish_successful_image_load(
    request_container_navigation_url_empty: bool,
) -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.source_url = RustImageOpenUrlTarget::SessionImage;
    transition.displayed_location = RustImageOpenDisplayedLocationTarget::SessionLocation;
    transition.container_navigation_url = if request_container_navigation_url_empty {
        RustImageOpenUrlTarget::DerivedContainerNavigation
    } else {
        RustImageOpenUrlTarget::SessionContainerNavigation
    };
    transition.error_string = RustImageOpenErrorStringTarget::Clear;
    transition.status = RustImageOpenStatusTarget::Ready;
    transition.effects.update_page_navigation = true;
    transition
}

fn rust_image_open_finish_container_navigation_load_with_error() -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.source_url = RustImageOpenUrlTarget::Container;
    transition.container_navigation_url = RustImageOpenUrlTarget::Container;
    transition.effects.clear_image = true;
    transition.effects.prepare_failed_container = true;
    transition
}

fn rust_image_open_finish_replacement_load_with_error(
    displayed_url_empty: bool,
) -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.status = RustImageOpenStatusTarget::Ready;
    if !displayed_url_empty {
        transition.source_url = RustImageOpenUrlTarget::Displayed;
    }
    transition.effects.update_page_navigation = true;
    transition.effects.schedule_adjacent_image_predecode = true;
    transition
}

fn rust_image_open_finish_initial_load_with_error() -> RustImageOpenTransition {
    cleared_load_error_transition(false)
}

fn rust_image_open_finish_animation_load_with_error() -> RustImageOpenTransition {
    cleared_load_error_transition(true)
}

fn rust_image_open_failure_target(
    container_navigation_url_empty: bool,
    has_image: bool,
) -> RustImageOpenFailureTarget {
    if !container_navigation_url_empty {
        return RustImageOpenFailureTarget::ContainerNavigation;
    }
    if has_image {
        return RustImageOpenFailureTarget::Replacement;
    }

    RustImageOpenFailureTarget::Initial
}

fn cleared_load_error_transition(reset_zoom: bool) -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.effects.clear_image = true;
    transition.effects.reset_zoom = reset_zoom;
    transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    transition
}

fn tracked_load_finished_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    transition.clear_loading_container_navigation_url = true;
    transition.loading = RustImageOpenBoolTarget::False;
    transition
}

fn tracked_load_error_transition() -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.error_string = RustImageOpenErrorStringTarget::Provided;
    transition.status = RustImageOpenStatusTarget::Error;
    transition
}

fn empty_transition() -> RustImageOpenTransition {
    RustImageOpenTransition {
        source_url: RustImageOpenUrlTarget::Unchanged,
        displayed_location: RustImageOpenDisplayedLocationTarget::Unchanged,
        container_navigation_url: RustImageOpenUrlTarget::Unchanged,
        loading: RustImageOpenBoolTarget::Unchanged,
        status: RustImageOpenStatusTarget::Unchanged,
        error_string: RustImageOpenErrorStringTarget::Unchanged,
        clear_loading_container_navigation_url: false,
        effects: RustImageOpenEffects {
            clear_image: false,
            reset_zoom: false,
            update_page_navigation: false,
            schedule_adjacent_image_predecode: false,
            prepare_failed_container: false,
        },
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_begin_source_load(false, true);

        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::True);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Loading);
        assert!(transition.effects.clear_image);
        assert!(transition.effects.reset_zoom);
    }

    #[test]
    fn source_target_distinguishes_empty_source_from_loadable_source() {
        assert_eq!(
            rust_image_open_source_target(true),
            RustImageOpenSourceTarget::EmptySource
        );
        assert_eq!(
            rust_image_open_source_target(false),
            RustImageOpenSourceTarget::LoadSource
        );
    }

    #[test]
    fn replacement_source_load_preserves_image_and_enters_ready() {
        let transition = rust_image_open_begin_source_load(true, true);

        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::Unchanged
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::True);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(!transition.effects.clear_image);
        assert!(!transition.effects.reset_zoom);
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition = rust_image_open_finish_successful_image_load(false);

        assert_eq!(transition.source_url, RustImageOpenUrlTarget::SessionImage);
        assert_eq!(
            transition.displayed_location,
            RustImageOpenDisplayedLocationTarget::SessionLocation
        );
        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::SessionContainerNavigation
        );
        assert_eq!(
            transition.error_string,
            RustImageOpenErrorStringTarget::Clear
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::False);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(transition.clear_loading_container_navigation_url);
        assert!(transition.effects.update_page_navigation);
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = rust_image_open_finish_replacement_load_with_error(false);

        assert_eq!(transition.source_url, RustImageOpenUrlTarget::Displayed);
        assert_eq!(
            transition.error_string,
            RustImageOpenErrorStringTarget::Provided
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::False);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(transition.effects.update_page_navigation);
        assert!(transition.effects.schedule_adjacent_image_predecode);
    }

    #[test]
    fn initial_and_animation_errors_share_clear_policy_but_only_animation_resets_zoom() {
        let initial = rust_image_open_finish_initial_load_with_error();
        let animation = rust_image_open_finish_animation_load_with_error();

        assert!(initial.effects.clear_image);
        assert!(!initial.effects.reset_zoom);
        assert_eq!(
            initial.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.status, RustImageOpenStatusTarget::Error);

        assert!(animation.effects.clear_image);
        assert!(animation.effects.reset_zoom);
        assert_eq!(
            animation.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(animation.status, RustImageOpenStatusTarget::Error);
    }

    #[test]
    fn load_failure_target_prefers_container_navigation_then_replacement_then_initial() {
        assert_eq!(
            rust_image_open_failure_target(false, true),
            RustImageOpenFailureTarget::ContainerNavigation
        );
        assert_eq!(
            rust_image_open_failure_target(false, false),
            RustImageOpenFailureTarget::ContainerNavigation
        );
        assert_eq!(
            rust_image_open_failure_target(true, true),
            RustImageOpenFailureTarget::Replacement
        );
        assert_eq!(
            rust_image_open_failure_target(true, false),
            RustImageOpenFailureTarget::Initial
        );
    }
}
