// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy)]
    struct RustImageSpreadSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy)]
    struct RustImageSpreadSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy)]
    struct RustImageSpreadRectF {
        x: f64,
        y: f64,
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageSpreadSecondaryPageDecision {
        PrimaryOnly = 0,
        LoadNext = 1,
        KeepCurrentSecondary = 2,
    }

    extern "Rust" {
        #[cxx_name = "rustImageSpreadImageSize"]
        fn rust_image_spread_image_size(
            primary_size: RustImageSpreadSize,
            secondary_size: RustImageSpreadSize,
        ) -> RustImageSpreadSize;

        #[cxx_name = "rustImageSpreadScaledPageDisplaySize"]
        fn rust_image_spread_scaled_page_display_size(
            page_size: RustImageSpreadSize,
            spread_image_size: RustImageSpreadSize,
            spread_display_size: RustImageSpreadSizeF,
        ) -> RustImageSpreadSizeF;

        #[cxx_name = "rustImageSpreadPrimaryPageRect"]
        fn rust_image_spread_primary_page_rect(
            primary_display_size: RustImageSpreadSizeF,
            secondary_display_size: RustImageSpreadSizeF,
            spread_display_size: RustImageSpreadSizeF,
            right_to_left_reading: bool,
        ) -> RustImageSpreadRectF;

        #[cxx_name = "rustImageSpreadSecondaryPageRect"]
        fn rust_image_spread_secondary_page_rect(
            primary_display_size: RustImageSpreadSizeF,
            secondary_display_size: RustImageSpreadSizeF,
            spread_display_size: RustImageSpreadSizeF,
            right_to_left_reading: bool,
        ) -> RustImageSpreadRectF;

        #[cxx_name = "rustImageSpreadVisiblePageRect"]
        fn rust_image_spread_visible_page_rect(
            visible_rect: RustImageSpreadRectF,
            page_rect: RustImageSpreadRectF,
        ) -> RustImageSpreadRectF;

        #[cxx_name = "rustImageSpreadPageIsWide"]
        fn rust_image_spread_page_is_wide(image_size: RustImageSpreadSize) -> bool;

        #[cxx_name = "rustImageSpreadPreviousPageTarget"]
        fn rust_image_spread_previous_page_target(
            current_page_number: i32,
            secondary_page_visible: bool,
            previous_page_is_wide: bool,
        ) -> i32;

        #[cxx_name = "rustImageSpreadCurrentLastPageNumber"]
        fn rust_image_spread_current_last_page_number(
            current_page_number: i32,
            secondary_page_visible: bool,
        ) -> i32;

        #[cxx_name = "rustImageSpreadRelativePageTarget"]
        fn rust_image_spread_relative_page_target(
            current_page_number: i32,
            image_count: i32,
            offset: i32,
        ) -> i32;

        #[cxx_name = "rustImageSpreadNextPageTarget"]
        fn rust_image_spread_next_page_target(
            current_last_page_number: i32,
            image_count: i32,
        ) -> i32;

        #[cxx_name = "rustImageSpreadShouldBeginTransition"]
        fn rust_image_spread_should_begin_transition(
            two_page_mode_active: bool,
            current_page_number: i32,
            target_page_number: i32,
            image_count: i32,
        ) -> bool;

        #[cxx_name = "rustImageSpreadSecondaryPageDecision"]
        fn rust_image_spread_secondary_page_decision(
            two_page_mode_active: bool,
            current_page_number: i32,
            image_count: i32,
            primary_page_is_wide: bool,
            next_page_available: bool,
            next_page_is_wide: bool,
            current_secondary_matches_next: bool,
        ) -> RustImageSpreadSecondaryPageDecision;
    }
}

use ffi::{
    RustImageSpreadRectF, RustImageSpreadSecondaryPageDecision, RustImageSpreadSize,
    RustImageSpreadSizeF,
};

fn rust_image_spread_image_size(
    primary_size: RustImageSpreadSize,
    secondary_size: RustImageSpreadSize,
) -> RustImageSpreadSize {
    if size_empty(primary_size) || size_empty(secondary_size) {
        return primary_size;
    }

    RustImageSpreadSize {
        width: primary_size.width.saturating_add(secondary_size.width),
        height: primary_size.height.max(secondary_size.height),
    }
}

fn rust_image_spread_scaled_page_display_size(
    page_size: RustImageSpreadSize,
    spread_image_size: RustImageSpreadSize,
    spread_display_size: RustImageSpreadSizeF,
) -> RustImageSpreadSizeF {
    if size_empty(page_size) || size_empty(spread_image_size) {
        return invalid_size_f();
    }

    let scale = spread_display_size.width / f64::from(spread_image_size.width);
    RustImageSpreadSizeF {
        width: f64::from(page_size.width) * scale,
        height: f64::from(page_size.height) * scale,
    }
}

fn rust_image_spread_primary_page_rect(
    primary_display_size: RustImageSpreadSizeF,
    secondary_display_size: RustImageSpreadSizeF,
    spread_display_size: RustImageSpreadSizeF,
    right_to_left_reading: bool,
) -> RustImageSpreadRectF {
    let x = if right_to_left_reading {
        secondary_display_size.width
    } else {
        0.0
    };
    page_rect(x, primary_display_size, spread_display_size)
}

fn rust_image_spread_secondary_page_rect(
    primary_display_size: RustImageSpreadSizeF,
    secondary_display_size: RustImageSpreadSizeF,
    spread_display_size: RustImageSpreadSizeF,
    right_to_left_reading: bool,
) -> RustImageSpreadRectF {
    let x = if right_to_left_reading {
        0.0
    } else {
        primary_display_size.width
    };
    page_rect(x, secondary_display_size, spread_display_size)
}

fn rust_image_spread_visible_page_rect(
    visible_rect: RustImageSpreadRectF,
    page_rect: RustImageSpreadRectF,
) -> RustImageSpreadRectF {
    translated_rect(
        intersect_rect_f(visible_rect, page_rect),
        -page_rect.x,
        -page_rect.y,
    )
}

fn rust_image_spread_page_is_wide(image_size: RustImageSpreadSize) -> bool {
    !size_empty(image_size) && image_size.width > image_size.height
}

fn rust_image_spread_previous_page_target(
    current_page_number: i32,
    secondary_page_visible: bool,
    previous_page_is_wide: bool,
) -> i32 {
    if current_page_number <= 0 {
        return 0;
    }

    if current_page_number <= 2 {
        return 1;
    }

    let offset = if secondary_page_visible && !previous_page_is_wide {
        -2
    } else {
        -1
    };
    current_page_number + offset
}

fn rust_image_spread_current_last_page_number(
    current_page_number: i32,
    secondary_page_visible: bool,
) -> i32 {
    if current_page_number <= 0 {
        return 0;
    }

    if secondary_page_visible {
        current_page_number.saturating_add(1)
    } else {
        current_page_number
    }
}

fn rust_image_spread_relative_page_target(
    current_page_number: i32,
    image_count: i32,
    offset: i32,
) -> i32 {
    let Some(target_page) = current_page_number.checked_add(offset) else {
        return 0;
    };
    if target_page < 1 || target_page > image_count {
        return 0;
    }

    target_page
}

fn rust_image_spread_next_page_target(current_last_page_number: i32, image_count: i32) -> i32 {
    let target_page_number = current_last_page_number + 1;
    if target_page_number > image_count {
        return 0;
    }

    target_page_number
}

fn rust_image_spread_should_begin_transition(
    two_page_mode_active: bool,
    current_page_number: i32,
    target_page_number: i32,
    image_count: i32,
) -> bool {
    two_page_mode_active
        && current_page_number > 0
        && target_page_number > 0
        && target_page_number <= image_count
        && target_page_number != current_page_number
}

fn rust_image_spread_secondary_page_decision(
    two_page_mode_active: bool,
    current_page_number: i32,
    image_count: i32,
    primary_page_is_wide: bool,
    next_page_available: bool,
    next_page_is_wide: bool,
    current_secondary_matches_next: bool,
) -> RustImageSpreadSecondaryPageDecision {
    let next_page_number = current_page_number + 1;
    if !two_page_mode_active
        || current_page_number == 1
        || primary_page_is_wide
        || next_page_number <= 1
        || next_page_number > image_count
        || !next_page_available
        || next_page_is_wide
    {
        return RustImageSpreadSecondaryPageDecision::PrimaryOnly;
    }

    if current_secondary_matches_next {
        RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary
    } else {
        RustImageSpreadSecondaryPageDecision::LoadNext
    }
}

fn size_empty(size: RustImageSpreadSize) -> bool {
    size.width <= 0 || size.height <= 0
}

fn invalid_size_f() -> RustImageSpreadSizeF {
    RustImageSpreadSizeF {
        width: -1.0,
        height: -1.0,
    }
}

fn page_rect(
    x: f64,
    page_size: RustImageSpreadSizeF,
    spread_size: RustImageSpreadSizeF,
) -> RustImageSpreadRectF {
    RustImageSpreadRectF {
        x,
        y: positive_or_zero((spread_size.height - page_size.height) / 2.0),
        width: page_size.width,
        height: page_size.height,
    }
}

fn intersect_rect_f(
    left: RustImageSpreadRectF,
    right: RustImageSpreadRectF,
) -> RustImageSpreadRectF {
    let x1 = left.x.max(right.x);
    let y1 = left.y.max(right.y);
    let x2 = rect_right(left).min(rect_right(right));
    let y2 = rect_bottom(left).min(rect_bottom(right));
    if x2 <= x1 || y2 <= y1 {
        return empty_rect_f();
    }

    RustImageSpreadRectF {
        x: x1,
        y: y1,
        width: x2 - x1,
        height: y2 - y1,
    }
}

fn translated_rect(rect: RustImageSpreadRectF, dx: f64, dy: f64) -> RustImageSpreadRectF {
    RustImageSpreadRectF {
        x: rect.x + dx,
        y: rect.y + dy,
        width: rect.width,
        height: rect.height,
    }
}

fn rect_right(rect: RustImageSpreadRectF) -> f64 {
    rect.x + rect.width
}

fn rect_bottom(rect: RustImageSpreadRectF) -> f64 {
    rect.y + rect.height
}

fn empty_rect_f() -> RustImageSpreadRectF {
    RustImageSpreadRectF {
        x: 0.0,
        y: 0.0,
        width: 0.0,
        height: 0.0,
    }
}

fn positive_or_zero(value: f64) -> f64 {
    if value > 0.0 { value } else { 0.0 }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn size(width: i32, height: i32) -> RustImageSpreadSize {
        RustImageSpreadSize { width, height }
    }

    fn size_f(width: f64, height: f64) -> RustImageSpreadSizeF {
        RustImageSpreadSizeF { width, height }
    }

    fn rect_f(x: f64, y: f64, width: f64, height: f64) -> RustImageSpreadRectF {
        RustImageSpreadRectF {
            x,
            y,
            width,
            height,
        }
    }

    fn assert_rect_eq(
        actual: RustImageSpreadRectF,
        expected_x: f64,
        expected_y: f64,
        expected_width: f64,
        expected_height: f64,
    ) {
        assert_eq!(actual.x, expected_x);
        assert_eq!(actual.y, expected_y);
        assert_eq!(actual.width, expected_width);
        assert_eq!(actual.height, expected_height);
    }

    #[test]
    fn spread_size_combines_page_widths_and_keeps_tallest_height() {
        let spread = rust_image_spread_image_size(size(800, 1200), size(700, 1000));

        assert_eq!(spread.width, 1500);
        assert_eq!(spread.height, 1200);
    }

    #[test]
    fn spread_size_uses_primary_size_until_secondary_page_is_available() {
        let spread = rust_image_spread_image_size(size(800, 1200), size(-1, -1));

        assert_eq!(spread.width, 800);
        assert_eq!(spread.height, 1200);
    }

    #[test]
    fn scaled_page_size_uses_spread_width_ratio() {
        let display = rust_image_spread_scaled_page_display_size(
            size(800, 1200),
            size(1500, 1200),
            size_f(750.0, 600.0),
        );

        assert_eq!(display.width, 400.0);
        assert_eq!(display.height, 600.0);
    }

    #[test]
    fn scaled_page_size_is_invalid_without_page_or_spread_size() {
        let display = rust_image_spread_scaled_page_display_size(
            size(-1, -1),
            size(1500, 1200),
            size_f(750.0, 600.0),
        );

        assert_eq!(display.width, -1.0);
        assert_eq!(display.height, -1.0);
    }

    #[test]
    fn page_rects_place_primary_before_secondary_in_left_to_right_mode() {
        let primary = rust_image_spread_primary_page_rect(
            size_f(400.0, 600.0),
            size_f(350.0, 500.0),
            size_f(750.0, 600.0),
            false,
        );
        let secondary = rust_image_spread_secondary_page_rect(
            size_f(400.0, 600.0),
            size_f(350.0, 500.0),
            size_f(750.0, 600.0),
            false,
        );

        assert_eq!(primary.x, 0.0);
        assert_eq!(primary.y, 0.0);
        assert_eq!(secondary.x, 400.0);
        assert_eq!(secondary.y, 50.0);
    }

    #[test]
    fn page_rects_place_secondary_before_primary_in_right_to_left_mode() {
        let primary = rust_image_spread_primary_page_rect(
            size_f(400.0, 600.0),
            size_f(350.0, 500.0),
            size_f(750.0, 600.0),
            true,
        );
        let secondary = rust_image_spread_secondary_page_rect(
            size_f(400.0, 600.0),
            size_f(350.0, 500.0),
            size_f(750.0, 600.0),
            true,
        );

        assert_eq!(primary.x, 350.0);
        assert_eq!(secondary.x, 0.0);
    }

    #[test]
    fn visible_page_rect_clips_to_page_and_uses_page_local_coordinates() {
        let rect = rust_image_spread_visible_page_rect(
            rect_f(200.0, 40.0, 300.0, 300.0),
            rect_f(100.0, 50.0, 400.0, 400.0),
        );

        assert_rect_eq(rect, 100.0, 0.0, 300.0, 290.0);
    }

    #[test]
    fn visible_page_rect_preserves_qrectf_empty_translation_behavior() {
        let rect = rust_image_spread_visible_page_rect(
            rect_f(0.0, 0.0, 10.0, 10.0),
            rect_f(20.0, 20.0, 5.0, 5.0),
        );

        assert_rect_eq(rect, -20.0, -20.0, 0.0, 0.0);
    }

    #[test]
    fn page_width_policy_requires_non_empty_landscape_size() {
        assert!(rust_image_spread_page_is_wide(size(1200, 800)));
        assert!(!rust_image_spread_page_is_wide(size(800, 800)));
        assert!(!rust_image_spread_page_is_wide(size(-1, -1)));
    }

    #[test]
    fn previous_page_target_keeps_cover_and_steps_by_spread() {
        assert_eq!(rust_image_spread_previous_page_target(0, false, false), 0);
        assert_eq!(rust_image_spread_previous_page_target(1, false, false), 1);
        assert_eq!(rust_image_spread_previous_page_target(2, false, false), 1);
        assert_eq!(rust_image_spread_previous_page_target(5, true, false), 3);
        assert_eq!(rust_image_spread_previous_page_target(5, true, true), 4);
        assert_eq!(rust_image_spread_previous_page_target(5, false, false), 4);
    }

    #[test]
    fn next_page_target_stops_after_last_page() {
        assert_eq!(rust_image_spread_next_page_target(2, 5), 3);
        assert_eq!(rust_image_spread_next_page_target(5, 5), 0);
    }

    #[test]
    fn current_last_page_tracks_secondary_page_visibility() {
        assert_eq!(rust_image_spread_current_last_page_number(0, false), 0);
        assert_eq!(rust_image_spread_current_last_page_number(2, false), 2);
        assert_eq!(rust_image_spread_current_last_page_number(2, true), 3);
    }

    #[test]
    fn relative_page_target_rejects_out_of_range_pages() {
        assert_eq!(rust_image_spread_relative_page_target(3, 5, -1), 2);
        assert_eq!(rust_image_spread_relative_page_target(3, 5, 1), 4);
        assert_eq!(rust_image_spread_relative_page_target(1, 5, -1), 0);
        assert_eq!(rust_image_spread_relative_page_target(5, 5, 1), 0);
    }

    #[test]
    fn transition_policy_requires_active_valid_different_target() {
        assert!(rust_image_spread_should_begin_transition(true, 2, 4, 6));
        assert!(!rust_image_spread_should_begin_transition(false, 2, 4, 6));
        assert!(!rust_image_spread_should_begin_transition(true, 2, 2, 6));
        assert!(!rust_image_spread_should_begin_transition(true, 2, 7, 6));
    }

    #[test]
    fn secondary_page_decision_selects_primary_keep_or_load() {
        assert_eq!(
            rust_image_spread_secondary_page_decision(true, 1, 4, false, true, false, false),
            RustImageSpreadSecondaryPageDecision::PrimaryOnly
        );
        assert_eq!(
            rust_image_spread_secondary_page_decision(true, 2, 4, true, true, false, false),
            RustImageSpreadSecondaryPageDecision::PrimaryOnly
        );
        assert_eq!(
            rust_image_spread_secondary_page_decision(true, 2, 4, false, true, true, false),
            RustImageSpreadSecondaryPageDecision::PrimaryOnly
        );
        assert_eq!(
            rust_image_spread_secondary_page_decision(true, 2, 4, false, true, false, true),
            RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary
        );
        assert_eq!(
            rust_image_spread_secondary_page_decision(true, 2, 4, false, true, false, false),
            RustImageSpreadSecondaryPageDecision::LoadNext
        );
    }
}
