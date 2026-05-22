// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::{
    RustPredecodeQueuedLoadPlan, RustPredecodeQueuedLoadState, RustPredecodeWindowLoadState,
};

pub(super) fn missing_window_load_indices(states: Vec<RustPredecodeWindowLoadState>) -> Vec<usize> {
    states
        .into_iter()
        .enumerate()
        .filter_map(|(index, state)| {
            if state.displayed || state.cached || state.in_flight {
                return None;
            }

            Some(index)
        })
        .collect()
}

pub(super) fn next_queued_load_plan(
    states: Vec<RustPredecodeQueuedLoadState>,
) -> RustPredecodeQueuedLoadPlan {
    let state_count = states.len();
    for (index, state) in states.into_iter().enumerate() {
        if state.valid && state.in_window && !state.cached && !state.in_flight {
            return queued_load_plan(index);
        }
    }

    missing_queued_load_plan(state_count)
}

fn queued_load_plan(index: usize) -> RustPredecodeQueuedLoadPlan {
    RustPredecodeQueuedLoadPlan {
        found: true,
        index,
        discard_count: index.saturating_add(1),
    }
}

fn missing_queued_load_plan(discard_count: usize) -> RustPredecodeQueuedLoadPlan {
    RustPredecodeQueuedLoadPlan {
        found: false,
        index: 0,
        discard_count,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn missing_window_load_indices_skip_displayed_cached_and_in_flight_urls() {
        assert_eq!(
            missing_window_load_indices(vec![
                window_load_state(true, false, false),
                window_load_state(false, true, false),
                window_load_state(false, false, true),
                window_load_state(false, false, false),
                window_load_state(false, false, false),
            ]),
            vec![3, 4]
        );
    }

    #[test]
    fn next_queued_load_plan_skips_invalid_stale_and_cached_requests() {
        assert_eq!(
            next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
                queued_load_state(true, true, true),
                queued_load_state(true, true, false),
            ]),
            queued_load_plan(3)
        );
        assert_eq!(
            next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
            ]),
            missing_queued_load_plan(2)
        );
    }

    #[test]
    fn next_queued_load_plan_skips_in_flight_requests() {
        assert_eq!(
            next_queued_load_plan(vec![
                queued_load_state_with_in_flight(true, true, false, true),
                queued_load_state(true, true, false),
            ]),
            queued_load_plan(1)
        );
    }

    fn window_load_state(
        displayed: bool,
        cached: bool,
        in_flight: bool,
    ) -> RustPredecodeWindowLoadState {
        RustPredecodeWindowLoadState {
            displayed,
            cached,
            in_flight,
        }
    }

    fn queued_load_state(
        valid: bool,
        in_window: bool,
        cached: bool,
    ) -> RustPredecodeQueuedLoadState {
        queued_load_state_with_in_flight(valid, in_window, cached, false)
    }

    fn queued_load_state_with_in_flight(
        valid: bool,
        in_window: bool,
        cached: bool,
        in_flight: bool,
    ) -> RustPredecodeQueuedLoadState {
        RustPredecodeQueuedLoadState {
            valid,
            in_window,
            cached,
            in_flight,
        }
    }
}
