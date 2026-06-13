// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const MINIMUM_MANUAL_ZOOM_PERCENT: f64 = 10.0;
const MANUAL_ZOOM_LOGICAL_LONG_EDGE_LIMIT: f64 = 65_536.0;
const MANUAL_ZOOM_VIEWPORT_LONG_EDGE_MULTIPLIER: f64 = 8.0;
const MANUAL_ZOOM_STEP_FACTOR: f64 = 1.1;
const ZOOM_EPSILON: f64 = 0.001;

#[cxx::bridge(namespace = "kiriview")]
mod ffi {
    #[derive(Clone, Copy, PartialEq, Eq)]
    enum RustImageZoomMode {
        Fit = 0,
        FitHeight = 1,
        FitWidth = 2,
        Manual = 3,
    }

    #[derive(Clone, Copy)]
    struct RustZoomSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy)]
    struct RustZoomSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy)]
    struct RustImageZoomState {
        image_width: i32,
        image_height: i32,
        viewport_width: f64,
        viewport_height: f64,
        display_width: f64,
        display_height: f64,
        zoom_percent: f64,
        zoom_mode: RustImageZoomMode,
    }

    #[derive(Clone, Copy)]
    struct RustImageZoomStateChange {
        changed: bool,
        state: RustImageZoomState,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageZoomChangeSet {
        image_size_changed: bool,
        viewport_size_changed: bool,
        zoom_mode_changed: bool,
        zoom_percent_changed: bool,
        display_size_changed: bool,
        maximum_manual_zoom_percent_changed: bool,
        display_projection_update_needed: bool,
    }

    #[derive(Clone, Copy)]
    struct RustLoadedImageZoom {
        zoom_mode: RustImageZoomMode,
        zoom_percent: f64,
        display_width: f64,
        display_height: f64,
    }

    extern "Rust" {
        #[cxx_name = "rustImageZoomApproximatelyEqual"]
        fn rust_image_zoom_approximately_equal(left: f64, right: f64) -> bool;

        #[cxx_name = "rustImageZoomSizeApproximatelyEqual"]
        fn rust_image_zoom_size_approximately_equal(
            left: RustZoomSizeF,
            right: RustZoomSizeF,
        ) -> bool;

        #[cxx_name = "rustImageZoomMinimumManualZoomPercent"]
        fn rust_image_zoom_minimum_manual_zoom_percent() -> f64;

        #[cxx_name = "rustImageZoomManualZoomPercentPropertyValue"]
        fn rust_image_zoom_manual_zoom_percent_property_value(zoom_percent: f64) -> i32;

        #[cxx_name = "rustImageZoomMaximumManualZoomPercent"]
        fn rust_image_zoom_maximum_manual_zoom_percent(
            state: RustImageZoomState,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustImageZoomClampedManualZoomPercent"]
        fn rust_image_zoom_clamped_manual_zoom_percent(
            state: RustImageZoomState,
            zoom_percent: f64,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustImageZoomManualZoomStepFactor"]
        fn rust_image_zoom_manual_zoom_step_factor() -> f64;

        #[cxx_name = "rustImageZoomSteppedManualZoomPercent"]
        fn rust_image_zoom_stepped_manual_zoom_percent(
            state: RustImageZoomState,
            step_count: f64,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustImageZoomSetViewportSize"]
        fn rust_image_zoom_set_viewport_size(
            state: RustImageZoomState,
            viewport_size: RustZoomSizeF,
            device_pixel_ratio: f64,
        ) -> RustImageZoomStateChange;

        #[cxx_name = "rustImageZoomSetImageSize"]
        fn rust_image_zoom_set_image_size(
            state: RustImageZoomState,
            image_size: RustZoomSize,
            device_pixel_ratio: f64,
        ) -> RustImageZoomStateChange;

        #[cxx_name = "rustImageZoomSetManualZoomPercent"]
        fn rust_image_zoom_set_manual_zoom_percent(
            state: RustImageZoomState,
            zoom_percent: f64,
            device_pixel_ratio: f64,
        ) -> RustImageZoomStateChange;

        #[cxx_name = "rustImageZoomSetFitMode"]
        fn rust_image_zoom_set_fit_mode(
            state: RustImageZoomState,
            zoom_mode: RustImageZoomMode,
            device_pixel_ratio: f64,
        ) -> RustImageZoomStateChange;

        #[cxx_name = "rustImageZoomChangeSet"]
        fn rust_image_zoom_change_set(
            previous: RustImageZoomState,
            previous_device_pixel_ratio: f64,
            current: RustImageZoomState,
            current_device_pixel_ratio: f64,
            force_display_projection_update: bool,
        ) -> RustImageZoomChangeSet;

        #[cxx_name = "rustImageZoomResetZoom"]
        fn rust_image_zoom_reset_zoom(
            state: RustImageZoomState,
            device_pixel_ratio: f64,
        ) -> RustImageZoomState;

        #[cxx_name = "rustImageZoomPrepareImageContainer"]
        fn rust_image_zoom_prepare_image_container(
            state: RustImageZoomState,
            same_container: bool,
        ) -> RustImageZoomState;

        #[cxx_name = "rustImageZoomLoadedImageZoom"]
        fn rust_image_zoom_loaded_image_zoom(
            state: RustImageZoomState,
            same_container: bool,
            image_size: RustZoomSize,
            device_pixel_ratio: f64,
        ) -> RustLoadedImageZoom;

        #[cxx_name = "rustImageZoomSetLoadedSvgZoom"]
        fn rust_image_zoom_set_loaded_svg_zoom(
            state: RustImageZoomState,
            zoom: RustLoadedImageZoom,
        ) -> RustImageZoomState;

        #[cxx_name = "rustImageZoomUpdate"]
        fn rust_image_zoom_update(
            state: RustImageZoomState,
            device_pixel_ratio: f64,
        ) -> RustImageZoomState;

        #[cxx_name = "rustImageZoomFitZoomPercent"]
        fn rust_image_zoom_fit_zoom_percent(
            state: RustImageZoomState,
            zoom_mode: RustImageZoomMode,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustImageZoomFitZoomPercentForImageSize"]
        fn rust_image_zoom_fit_zoom_percent_for_image_size(
            state: RustImageZoomState,
            zoom_mode: RustImageZoomMode,
            image_size: RustZoomSize,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustImageZoomDisplaySizeForZoomPercent"]
        fn rust_image_zoom_display_size_for_zoom_percent(
            image_size: RustZoomSize,
            zoom_percent: f64,
            device_pixel_ratio: f64,
        ) -> RustZoomSizeF;
    }
}

use ffi::{
    RustImageZoomChangeSet, RustImageZoomMode, RustImageZoomState, RustImageZoomStateChange,
    RustLoadedImageZoom, RustZoomSize, RustZoomSizeF,
};

impl RustImageZoomState {
    fn image_size(self) -> RustZoomSize {
        RustZoomSize {
            width: self.image_width,
            height: self.image_height,
        }
    }

    fn viewport_size(self) -> RustZoomSizeF {
        RustZoomSizeF {
            width: self.viewport_width,
            height: self.viewport_height,
        }
    }

    fn with_display_size_for_zoom(
        mut self,
        image_size: RustZoomSize,
        zoom_percent: f64,
        device_pixel_ratio: f64,
    ) -> RustImageZoomState {
        let display_size = zoom_display_size(image_size, zoom_percent, device_pixel_ratio);
        self.display_width = display_size.width;
        self.display_height = display_size.height;
        self
    }

    fn with_current_display_size(self, device_pixel_ratio: f64) -> RustImageZoomState {
        self.with_display_size_for_zoom(self.image_size(), self.zoom_percent, device_pixel_ratio)
    }

    fn updated(mut self, device_pixel_ratio: f64) -> RustImageZoomState {
        if self.zoom_mode == RustImageZoomMode::Manual {
            self.zoom_percent = rust_image_zoom_clamped_manual_zoom_percent(
                self,
                self.zoom_percent,
                device_pixel_ratio,
            );
        } else {
            self.zoom_percent =
                rust_image_zoom_fit_zoom_percent(self, self.zoom_mode, device_pixel_ratio);
        }

        self.with_current_display_size(device_pixel_ratio)
    }
}

impl RustImageZoomStateChange {
    fn unchanged(state: RustImageZoomState) -> RustImageZoomStateChange {
        RustImageZoomStateChange {
            changed: false,
            state,
        }
    }

    fn changed(state: RustImageZoomState) -> RustImageZoomStateChange {
        RustImageZoomStateChange {
            changed: true,
            state,
        }
    }

    fn changed_and_updated(
        state: RustImageZoomState,
        device_pixel_ratio: f64,
    ) -> RustImageZoomStateChange {
        Self::changed(state.updated(device_pixel_ratio))
    }
}

fn zoom_display_size(
    image_size: RustZoomSize,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> RustZoomSizeF {
    rust_image_zoom_display_size_for_zoom_percent(image_size, zoom_percent, device_pixel_ratio)
}

fn rust_image_zoom_approximately_equal(left: f64, right: f64) -> bool {
    (left - right).abs() < ZOOM_EPSILON
}

fn rust_image_zoom_size_approximately_equal(left: RustZoomSizeF, right: RustZoomSizeF) -> bool {
    rust_image_zoom_approximately_equal(left.width, right.width)
        && rust_image_zoom_approximately_equal(left.height, right.height)
}

fn rust_image_zoom_minimum_manual_zoom_percent() -> f64 {
    MINIMUM_MANUAL_ZOOM_PERCENT
}

fn rust_image_zoom_manual_zoom_percent_property_value(zoom_percent: f64) -> i32 {
    if !zoom_percent.is_finite() {
        return MINIMUM_MANUAL_ZOOM_PERCENT as i32;
    }

    zoom_percent.ceil().clamp(0.0, f64::from(i32::MAX)) as i32
}

fn rust_image_zoom_maximum_manual_zoom_percent(
    state: RustImageZoomState,
    device_pixel_ratio: f64,
) -> f64 {
    maximum_manual_zoom_percent_for_image_size(state, state.image_size(), device_pixel_ratio)
}

fn rust_image_zoom_clamped_manual_zoom_percent(
    state: RustImageZoomState,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> f64 {
    clamped_manual_zoom_percent_for_image_size(
        state,
        zoom_percent,
        state.image_size(),
        device_pixel_ratio,
    )
}

fn rust_image_zoom_manual_zoom_step_factor() -> f64 {
    MANUAL_ZOOM_STEP_FACTOR
}

fn rust_image_zoom_stepped_manual_zoom_percent(
    state: RustImageZoomState,
    step_count: f64,
    device_pixel_ratio: f64,
) -> f64 {
    if !state.zoom_percent.is_finite() || state.zoom_percent <= 0.0 || !step_count.is_finite() {
        return state.zoom_percent;
    }

    let zoom_factor = MANUAL_ZOOM_STEP_FACTOR.powf(step_count);
    let zoom_percent = state.zoom_percent * zoom_factor;
    if zoom_factor.is_finite() && zoom_percent.is_finite() {
        return rust_image_zoom_clamped_manual_zoom_percent(
            state,
            zoom_percent,
            device_pixel_ratio,
        );
    }

    if step_count.is_sign_positive() {
        rust_image_zoom_maximum_manual_zoom_percent(state, device_pixel_ratio)
    } else {
        MINIMUM_MANUAL_ZOOM_PERCENT
    }
}

fn rust_image_zoom_set_viewport_size(
    mut state: RustImageZoomState,
    viewport_size: RustZoomSizeF,
    device_pixel_ratio: f64,
) -> RustImageZoomStateChange {
    let normalized_viewport_size = RustZoomSizeF {
        width: 0.0_f64.max(viewport_size.width),
        height: 0.0_f64.max(viewport_size.height),
    };
    if rust_image_zoom_size_approximately_equal(state.viewport_size(), normalized_viewport_size) {
        return RustImageZoomStateChange::unchanged(state);
    }

    state.viewport_width = normalized_viewport_size.width;
    state.viewport_height = normalized_viewport_size.height;
    RustImageZoomStateChange::changed_and_updated(state, device_pixel_ratio)
}

fn rust_image_zoom_set_image_size(
    mut state: RustImageZoomState,
    image_size: RustZoomSize,
    device_pixel_ratio: f64,
) -> RustImageZoomStateChange {
    if state.image_width == image_size.width && state.image_height == image_size.height {
        return RustImageZoomStateChange::unchanged(state);
    }

    state.image_width = image_size.width;
    state.image_height = image_size.height;
    RustImageZoomStateChange::changed_and_updated(state, device_pixel_ratio)
}

fn rust_image_zoom_set_manual_zoom_percent(
    mut state: RustImageZoomState,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> RustImageZoomStateChange {
    if !zoom_percent.is_finite() {
        return RustImageZoomStateChange::unchanged(state);
    }

    let clamped_zoom_percent =
        rust_image_zoom_clamped_manual_zoom_percent(state, zoom_percent, device_pixel_ratio);
    let zoom_changed =
        !rust_image_zoom_approximately_equal(state.zoom_percent, clamped_zoom_percent);
    let mode_changed = state.zoom_mode != RustImageZoomMode::Manual;

    if !zoom_changed && !mode_changed {
        return RustImageZoomStateChange::unchanged(state);
    }

    state.zoom_mode = RustImageZoomMode::Manual;
    state.zoom_percent = clamped_zoom_percent;
    RustImageZoomStateChange::changed(state.with_current_display_size(device_pixel_ratio))
}

fn rust_image_zoom_set_fit_mode(
    mut state: RustImageZoomState,
    zoom_mode: RustImageZoomMode,
    device_pixel_ratio: f64,
) -> RustImageZoomStateChange {
    if zoom_mode == RustImageZoomMode::Manual || state.zoom_mode == zoom_mode {
        return RustImageZoomStateChange::unchanged(state);
    }

    state.zoom_mode = zoom_mode;
    RustImageZoomStateChange::changed_and_updated(state, device_pixel_ratio)
}

fn rust_image_zoom_change_set(
    previous: RustImageZoomState,
    previous_device_pixel_ratio: f64,
    current: RustImageZoomState,
    current_device_pixel_ratio: f64,
    force_display_projection_update: bool,
) -> RustImageZoomChangeSet {
    let image_size_changed = previous.image_width != current.image_width
        || previous.image_height != current.image_height;
    let viewport_size_changed = !rust_image_zoom_size_approximately_equal(
        previous.viewport_size(),
        current.viewport_size(),
    );
    let zoom_mode_changed = previous.zoom_mode != current.zoom_mode;
    let zoom_percent_changed =
        !rust_image_zoom_approximately_equal(previous.zoom_percent, current.zoom_percent);
    let display_size_changed = !rust_image_zoom_size_approximately_equal(
        RustZoomSizeF {
            width: previous.display_width,
            height: previous.display_height,
        },
        RustZoomSizeF {
            width: current.display_width,
            height: current.display_height,
        },
    );
    let previous_maximum_manual_zoom_percent =
        rust_image_zoom_maximum_manual_zoom_percent(previous, previous_device_pixel_ratio);
    let current_maximum_manual_zoom_percent =
        rust_image_zoom_maximum_manual_zoom_percent(current, current_device_pixel_ratio);
    let maximum_manual_zoom_percent_changed = !rust_image_zoom_approximately_equal(
        previous_maximum_manual_zoom_percent,
        current_maximum_manual_zoom_percent,
    );
    let zoom_state_changed = image_size_changed
        || viewport_size_changed
        || zoom_mode_changed
        || zoom_percent_changed
        || display_size_changed
        || maximum_manual_zoom_percent_changed;

    RustImageZoomChangeSet {
        image_size_changed,
        viewport_size_changed,
        zoom_mode_changed,
        zoom_percent_changed,
        display_size_changed,
        maximum_manual_zoom_percent_changed,
        display_projection_update_needed: zoom_state_changed || force_display_projection_update,
    }
}

fn rust_image_zoom_reset_zoom(
    mut state: RustImageZoomState,
    device_pixel_ratio: f64,
) -> RustImageZoomState {
    state.zoom_mode = RustImageZoomMode::Fit;
    state.updated(device_pixel_ratio)
}

fn rust_image_zoom_prepare_image_container(
    mut state: RustImageZoomState,
    same_container: bool,
) -> RustImageZoomState {
    if !same_container {
        state.zoom_mode = RustImageZoomMode::Fit;
    }
    state
}

fn rust_image_zoom_loaded_image_zoom(
    state: RustImageZoomState,
    same_container: bool,
    image_size: RustZoomSize,
    device_pixel_ratio: f64,
) -> RustLoadedImageZoom {
    let loaded_zoom_mode = if same_container {
        state.zoom_mode
    } else {
        RustImageZoomMode::Fit
    };
    let loaded_zoom_percent = if loaded_zoom_mode == RustImageZoomMode::Manual {
        clamped_manual_zoom_percent_for_image_size(
            state,
            state.zoom_percent,
            image_size,
            device_pixel_ratio,
        )
    } else {
        rust_image_zoom_fit_zoom_percent_for_image_size(
            state,
            loaded_zoom_mode,
            image_size,
            device_pixel_ratio,
        )
    };
    let display_size = zoom_display_size(image_size, loaded_zoom_percent, device_pixel_ratio);

    RustLoadedImageZoom {
        zoom_mode: loaded_zoom_mode,
        zoom_percent: loaded_zoom_percent,
        display_width: display_size.width,
        display_height: display_size.height,
    }
}

fn rust_image_zoom_set_loaded_svg_zoom(
    mut state: RustImageZoomState,
    zoom: RustLoadedImageZoom,
) -> RustImageZoomState {
    state.zoom_mode = zoom.zoom_mode;
    state.zoom_percent = zoom.zoom_percent;
    state.display_width = 0.0_f64.max(zoom.display_width);
    state.display_height = 0.0_f64.max(zoom.display_height);
    state
}

fn rust_image_zoom_update(
    state: RustImageZoomState,
    device_pixel_ratio: f64,
) -> RustImageZoomState {
    state.updated(device_pixel_ratio)
}

fn rust_image_zoom_fit_zoom_percent(
    state: RustImageZoomState,
    zoom_mode: RustImageZoomMode,
    device_pixel_ratio: f64,
) -> f64 {
    rust_image_zoom_fit_zoom_percent_for_image_size(
        state,
        zoom_mode,
        state.image_size(),
        device_pixel_ratio,
    )
}

fn normalized_device_pixel_ratio(device_pixel_ratio: f64) -> Option<f64> {
    if device_pixel_ratio.is_finite() && device_pixel_ratio > 0.0 {
        Some(device_pixel_ratio)
    } else {
        None
    }
}

fn safe_minimum_zoom_cap(fit_zoom_percent: f64) -> f64 {
    if fit_zoom_percent.is_finite() && fit_zoom_percent > MINIMUM_MANUAL_ZOOM_PERCENT {
        fit_zoom_percent
    } else {
        MINIMUM_MANUAL_ZOOM_PERCENT
    }
}

fn viewport_long_edge(viewport_size: RustZoomSizeF) -> f64 {
    let width = if viewport_size.width.is_finite() {
        0.0_f64.max(viewport_size.width)
    } else {
        0.0
    };
    let height = if viewport_size.height.is_finite() {
        0.0_f64.max(viewport_size.height)
    } else {
        0.0
    };

    width.max(height)
}

fn maximum_manual_zoom_percent_for_image_size(
    state: RustImageZoomState,
    image_size: RustZoomSize,
    device_pixel_ratio: f64,
) -> f64 {
    let fit_zoom_percent = rust_image_zoom_fit_zoom_percent_for_image_size(
        state,
        RustImageZoomMode::Fit,
        image_size,
        device_pixel_ratio,
    );
    let minimum_cap = safe_minimum_zoom_cap(fit_zoom_percent);
    let Some(device_pixel_ratio) = normalized_device_pixel_ratio(device_pixel_ratio) else {
        return minimum_cap;
    };

    let image_long_edge = image_size.width.max(image_size.height);
    if image_long_edge <= 0 {
        return minimum_cap;
    }

    let max_display_long_edge = MANUAL_ZOOM_LOGICAL_LONG_EDGE_LIMIT
        .max(viewport_long_edge(state.viewport_size()) * MANUAL_ZOOM_VIEWPORT_LONG_EDGE_MULTIPLIER);
    let calculated_cap =
        max_display_long_edge * device_pixel_ratio * 100.0 / f64::from(image_long_edge);
    if calculated_cap.is_finite() && calculated_cap > minimum_cap {
        calculated_cap
    } else {
        minimum_cap
    }
}

fn clamped_manual_zoom_percent_for_image_size(
    state: RustImageZoomState,
    zoom_percent: f64,
    image_size: RustZoomSize,
    device_pixel_ratio: f64,
) -> f64 {
    if !zoom_percent.is_finite() {
        return zoom_percent;
    }

    zoom_percent.clamp(
        MINIMUM_MANUAL_ZOOM_PERCENT,
        maximum_manual_zoom_percent_for_image_size(state, image_size, device_pixel_ratio),
    )
}

fn rust_image_zoom_fit_zoom_percent_for_image_size(
    state: RustImageZoomState,
    zoom_mode: RustImageZoomMode,
    image_size: RustZoomSize,
    device_pixel_ratio: f64,
) -> f64 {
    let fallback_zoom_percent = device_pixel_ratio * 100.0;

    if image_size.width <= 0 || image_size.height <= 0 {
        return fallback_zoom_percent;
    }

    let fit_width_zoom_percent = || {
        if state.viewport_width <= 0.0 {
            fallback_zoom_percent
        } else {
            0.0_f64.max(
                state.viewport_width * device_pixel_ratio * 100.0 / f64::from(image_size.width),
            )
        }
    };
    let fit_height_zoom_percent = || {
        if state.viewport_height <= 0.0 {
            fallback_zoom_percent
        } else {
            0.0_f64.max(
                state.viewport_height * device_pixel_ratio * 100.0 / f64::from(image_size.height),
            )
        }
    };

    match zoom_mode {
        RustImageZoomMode::Fit => {
            if state.viewport_width <= 0.0 || state.viewport_height <= 0.0 {
                fallback_zoom_percent
            } else {
                fit_width_zoom_percent().min(fit_height_zoom_percent())
            }
        }
        RustImageZoomMode::FitHeight => fit_height_zoom_percent(),
        RustImageZoomMode::FitWidth => fit_width_zoom_percent(),
        RustImageZoomMode::Manual => state.zoom_percent,
        _ => fallback_zoom_percent,
    }
}

fn rust_image_zoom_display_size_for_zoom_percent(
    image_size: RustZoomSize,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> RustZoomSizeF {
    let (width, height) = crate::imagemath::display_size_for_zoom_percent(
        image_size.width,
        image_size.height,
        zoom_percent,
        device_pixel_ratio,
    );
    RustZoomSizeF { width, height }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn zoom_state() -> RustImageZoomState {
        RustImageZoomState {
            image_width: 200,
            image_height: 100,
            viewport_width: 400.0,
            viewport_height: 300.0,
            display_width: 400.0,
            display_height: 200.0,
            zoom_percent: 200.0,
            zoom_mode: RustImageZoomMode::Fit,
        }
    }

    #[test]
    fn manual_zoom_percent_property_value_is_a_bounded_integer() {
        assert_eq!(rust_image_zoom_manual_zoom_percent_property_value(10.0), 10);
        assert_eq!(rust_image_zoom_manual_zoom_percent_property_value(10.1), 11);
        assert_eq!(rust_image_zoom_manual_zoom_percent_property_value(-4.5), 0);
        assert_eq!(
            rust_image_zoom_manual_zoom_percent_property_value(f64::from(i32::MAX) + 10_000.0),
            i32::MAX
        );
    }

    #[test]
    fn manual_zoom_percent_property_value_uses_minimum_for_non_finite_values() {
        assert_eq!(
            rust_image_zoom_manual_zoom_percent_property_value(f64::NAN),
            MINIMUM_MANUAL_ZOOM_PERCENT as i32
        );
        assert_eq!(
            rust_image_zoom_manual_zoom_percent_property_value(f64::INFINITY),
            MINIMUM_MANUAL_ZOOM_PERCENT as i32
        );
    }

    #[test]
    fn change_set_reports_zoom_snapshot_differences() {
        let previous = zoom_state();
        let current = RustImageZoomState {
            image_width: 300,
            viewport_width: 500.0,
            display_width: 500.0,
            zoom_percent: 250.0,
            zoom_mode: RustImageZoomMode::FitWidth,
            ..previous
        };

        let changes = rust_image_zoom_change_set(previous, 1.0, current, 1.0, false);

        assert!(changes.image_size_changed);
        assert!(changes.viewport_size_changed);
        assert!(changes.zoom_mode_changed);
        assert!(changes.zoom_percent_changed);
        assert!(changes.display_size_changed);
        assert!(changes.maximum_manual_zoom_percent_changed);
        assert!(changes.display_projection_update_needed);
    }

    #[test]
    fn change_set_tracks_maximum_manual_zoom_changes_from_render_context() {
        let state = zoom_state();

        let changes = rust_image_zoom_change_set(state, 1.0, state, 2.0, false);

        assert!(!changes.image_size_changed);
        assert!(!changes.viewport_size_changed);
        assert!(!changes.zoom_mode_changed);
        assert!(!changes.zoom_percent_changed);
        assert!(!changes.display_size_changed);
        assert!(changes.maximum_manual_zoom_percent_changed);
        assert!(changes.display_projection_update_needed);
    }

    #[test]
    fn change_set_can_force_visible_tile_decode_without_zoom_changes() {
        let state = zoom_state();

        let changes = rust_image_zoom_change_set(state, 1.0, state, 1.0, true);

        assert!(!changes.image_size_changed);
        assert!(!changes.viewport_size_changed);
        assert!(!changes.zoom_mode_changed);
        assert!(!changes.zoom_percent_changed);
        assert!(!changes.display_size_changed);
        assert!(!changes.maximum_manual_zoom_percent_changed);
        assert!(changes.display_projection_update_needed);
    }
}
