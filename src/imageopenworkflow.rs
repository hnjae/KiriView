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
    struct RustImageOpenSourceLoadPlan {
        finish_spread_transition: bool,
        reset_right_to_left_reading: bool,
        clear_loading_container_navigation_url: bool,
        update_container_navigation_url: bool,
        cancel_navigation_and_predecode: bool,
        clear_secondary_page: bool,
        set_loading_container_navigation_url: bool,
        set_source_url: bool,
        begin_open: bool,
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
        #[cxx_name = "rustImageOpenSourceLoadPlan"]
        fn rust_image_open_source_load_plan(
            source_url_changed: bool,
            preserve_two_page_spread_transition: bool,
            reset_right_to_left_reading: bool,
            container_navigation_url_empty: bool,
        ) -> RustImageOpenSourceLoadPlan;

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

        #[cxx_name = "rustImageOpenFinishLoadWithError"]
        fn rust_image_open_finish_load_with_error(
            container_navigation_url_empty: bool,
            has_image: bool,
            displayed_url_empty: bool,
        ) -> RustImageOpenTransition;
    }
}

use ffi::{
    RustImageOpenBoolTarget, RustImageOpenDisplayedLocationTarget, RustImageOpenEffects,
    RustImageOpenErrorStringTarget, RustImageOpenSourceLoadPlan, RustImageOpenStatusTarget,
    RustImageOpenTransition, RustImageOpenUrlTarget,
};

fn rust_image_open_source_load_plan(
    source_url_changed: bool,
    preserve_two_page_spread_transition: bool,
    reset_right_to_left_reading: bool,
    container_navigation_url_empty: bool,
) -> RustImageOpenSourceLoadPlan {
    let mut plan = empty_source_load_plan();
    plan.finish_spread_transition = !preserve_two_page_spread_transition;
    plan.reset_right_to_left_reading = reset_right_to_left_reading;

    if !source_url_changed {
        plan.clear_loading_container_navigation_url = true;
        plan.update_container_navigation_url = !container_navigation_url_empty;
        return plan;
    }

    plan.cancel_navigation_and_predecode = true;
    plan.clear_secondary_page = true;
    plan.set_loading_container_navigation_url = true;
    plan.set_source_url = true;
    plan.begin_open = true;
    plan
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
    transition.effects.schedule_adjacent_image_predecode = true;
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

fn rust_image_open_finish_load_with_error(
    container_navigation_url_empty: bool,
    has_image: bool,
    displayed_url_empty: bool,
) -> RustImageOpenTransition {
    if !container_navigation_url_empty {
        return rust_image_open_finish_container_navigation_load_with_error();
    }
    if has_image {
        return rust_image_open_finish_replacement_load_with_error(displayed_url_empty);
    }

    rust_image_open_finish_initial_load_with_error()
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

fn empty_source_load_plan() -> RustImageOpenSourceLoadPlan {
    RustImageOpenSourceLoadPlan {
        finish_spread_transition: false,
        reset_right_to_left_reading: false,
        clear_loading_container_navigation_url: false,
        update_container_navigation_url: false,
        cancel_navigation_and_predecode: false,
        clear_secondary_page: false,
        set_loading_container_navigation_url: false,
        set_source_url: false,
        begin_open: false,
    }
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
    fn unchanged_source_load_clears_loading_and_optionally_updates_container() {
        let plan = rust_image_open_source_load_plan(false, false, true, false);

        assert!(plan.finish_spread_transition);
        assert!(plan.reset_right_to_left_reading);
        assert!(plan.clear_loading_container_navigation_url);
        assert!(plan.update_container_navigation_url);
        assert!(!plan.cancel_navigation_and_predecode);
        assert!(!plan.clear_secondary_page);
        assert!(!plan.set_loading_container_navigation_url);
        assert!(!plan.set_source_url);
        assert!(!plan.begin_open);
    }

    #[test]
    fn changed_source_load_starts_new_load_without_container_navigation_update() {
        let plan = rust_image_open_source_load_plan(true, false, false, false);

        assert!(plan.finish_spread_transition);
        assert!(!plan.reset_right_to_left_reading);
        assert!(!plan.clear_loading_container_navigation_url);
        assert!(!plan.update_container_navigation_url);
        assert!(plan.cancel_navigation_and_predecode);
        assert!(plan.clear_secondary_page);
        assert!(plan.set_loading_container_navigation_url);
        assert!(plan.set_source_url);
        assert!(plan.begin_open);
    }

    #[test]
    fn source_load_plan_can_preserve_active_spread_transition() {
        let plan = rust_image_open_source_load_plan(true, true, true, true);

        assert!(!plan.finish_spread_transition);
        assert!(plan.reset_right_to_left_reading);
        assert!(plan.begin_open);
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
        assert!(transition.effects.schedule_adjacent_image_predecode);
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
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container = rust_image_open_finish_load_with_error(false, true, false);
        assert_eq!(container.source_url, RustImageOpenUrlTarget::Container);
        assert!(container.effects.clear_image);
        assert!(container.effects.prepare_failed_container);
        assert_eq!(container.status, RustImageOpenStatusTarget::Error);

        let replacement = rust_image_open_finish_load_with_error(true, true, false);
        assert_eq!(replacement.source_url, RustImageOpenUrlTarget::Displayed);
        assert!(replacement.effects.update_page_navigation);
        assert!(replacement.effects.schedule_adjacent_image_predecode);
        assert_eq!(replacement.status, RustImageOpenStatusTarget::Ready);

        let initial = rust_image_open_finish_load_with_error(true, false, true);
        assert_eq!(initial.source_url, RustImageOpenUrlTarget::Unchanged);
        assert!(initial.effects.clear_image);
        assert_eq!(
            initial.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.status, RustImageOpenStatusTarget::Error);
    }
}
