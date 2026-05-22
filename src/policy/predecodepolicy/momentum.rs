// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::{
    RustPredecodeMomentumDirection, RustPredecodeMomentumMode, RustPredecodeMomentumState,
    RustPredecodeMomentumUpdateInput,
    config::{
        PREDECODE_BIASED_NAVIGATION_MSEC, PREDECODE_NEUTRAL_NAVIGATION_MSEC,
        PREDECODE_SCRUBBING_NAVIGATION_MSEC, PREDECODE_SCRUBBING_PAGE_JUMP,
    },
};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct PredecodeMomentumInput {
    previous_page_index: i32,
    current_page_index: i32,
    elapsed_msec: i64,
    same_direction_move_count: i32,
}

pub(super) fn updated_momentum_state(
    input: RustPredecodeMomentumUpdateInput,
) -> RustPredecodeMomentumState {
    let mut state = input.state;
    state.mode = RustPredecodeMomentumMode::Neutral;

    if input.page_index < 0 {
        return state;
    }

    if state.last_page_index < 0 || state.last_navigation_msec < 0 {
        state.last_page_index = input.page_index;
        state.last_navigation_msec = input.monotonic_msec;
        return state;
    }

    if input.page_index == state.last_page_index {
        return state;
    }

    let elapsed_msec = input
        .monotonic_msec
        .saturating_sub(state.last_navigation_msec);
    let direction = if input.page_index > state.last_page_index {
        RustPredecodeMomentumDirection::Next
    } else {
        RustPredecodeMomentumDirection::Previous
    };
    if direction == state.last_direction && elapsed_msec <= PREDECODE_BIASED_NAVIGATION_MSEC {
        state.same_direction_move_count = state.same_direction_move_count.saturating_add(1);
    } else {
        state.same_direction_move_count = 1;
    }

    state.mode = momentum_mode(PredecodeMomentumInput {
        previous_page_index: state.last_page_index,
        current_page_index: input.page_index,
        elapsed_msec,
        same_direction_move_count: state.same_direction_move_count,
    });
    state.last_direction = direction;
    state.last_page_index = input.page_index;
    state.last_navigation_msec = input.monotonic_msec;
    state
}

fn momentum_mode(input: PredecodeMomentumInput) -> RustPredecodeMomentumMode {
    if input.previous_page_index < 0
        || input.current_page_index < 0
        || input.previous_page_index == input.current_page_index
        || input.elapsed_msec < 0
        || input.elapsed_msec >= PREDECODE_NEUTRAL_NAVIGATION_MSEC
    {
        return RustPredecodeMomentumMode::Neutral;
    }

    let page_delta = input
        .current_page_index
        .saturating_sub(input.previous_page_index);
    let next_direction = page_delta > 0;
    let repeated_fast_move = input.same_direction_move_count >= 2
        && input.elapsed_msec <= PREDECODE_SCRUBBING_NAVIGATION_MSEC;
    let short_large_jump = page_delta.abs() >= PREDECODE_SCRUBBING_PAGE_JUMP
        && input.elapsed_msec <= PREDECODE_BIASED_NAVIGATION_MSEC;
    if repeated_fast_move || short_large_jump {
        return if next_direction {
            RustPredecodeMomentumMode::ScrubbingNext
        } else {
            RustPredecodeMomentumMode::ScrubbingPrev
        };
    }

    if input.same_direction_move_count >= 2
        && input.elapsed_msec <= PREDECODE_BIASED_NAVIGATION_MSEC
    {
        return if next_direction {
            RustPredecodeMomentumMode::NextBiased
        } else {
            RustPredecodeMomentumMode::PrevBiased
        };
    }

    RustPredecodeMomentumMode::Neutral
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn momentum_mode_classifies_fast_repeated_and_large_page_moves() {
        assert_eq!(
            momentum_mode(momentum_input(4, 5, 300, 1)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            momentum_mode(momentum_input(4, 5, 300, 2)),
            RustPredecodeMomentumMode::NextBiased
        );
        assert_eq!(
            momentum_mode(momentum_input(5, 4, 300, 2)),
            RustPredecodeMomentumMode::PrevBiased
        );
        assert_eq!(
            momentum_mode(momentum_input(4, 5, 120, 2)),
            RustPredecodeMomentumMode::ScrubbingNext
        );
        assert_eq!(
            momentum_mode(momentum_input(10, 6, 300, 1)),
            RustPredecodeMomentumMode::ScrubbingPrev
        );
    }

    #[test]
    fn updated_momentum_state_tracks_repeated_direction_in_rust() {
        let state =
            updated_momentum_state(momentum_update_input(initial_momentum_state(), 4, 1000));
        assert_eq!(state.last_page_index, 4);
        assert_eq!(state.last_navigation_msec, 1000);
        assert_eq!(state.same_direction_move_count, 0);
        assert_eq!(state.last_direction, RustPredecodeMomentumDirection::None);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = updated_momentum_state(momentum_update_input(state, 5, 1300));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(state.last_direction, RustPredecodeMomentumDirection::Next);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = updated_momentum_state(momentum_update_input(state, 6, 1600));
        assert_eq!(state.same_direction_move_count, 2);
        assert_eq!(state.last_direction, RustPredecodeMomentumDirection::Next);
        assert_eq!(state.mode, RustPredecodeMomentumMode::NextBiased);
    }

    #[test]
    fn updated_momentum_state_resets_for_direction_changes_settling_and_invalid_pages() {
        let state = RustPredecodeMomentumState {
            last_page_index: 6,
            last_navigation_msec: 1000,
            same_direction_move_count: 2,
            last_direction: RustPredecodeMomentumDirection::Next,
            mode: RustPredecodeMomentumMode::NextBiased,
        };

        let state = updated_momentum_state(momentum_update_input(state, 5, 1200));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(
            state.last_direction,
            RustPredecodeMomentumDirection::Previous
        );
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = updated_momentum_state(momentum_update_input(state, 4, 2100));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = updated_momentum_state(momentum_update_input(state, -1, 2200));
        assert_eq!(state.last_page_index, 4);
        assert_eq!(state.last_navigation_msec, 2100);
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(
            state.last_direction,
            RustPredecodeMomentumDirection::Previous
        );
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);
    }

    #[test]
    fn momentum_mode_returns_neutral_for_invalid_or_settled_navigation() {
        assert_eq!(
            momentum_mode(momentum_input(-1, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            momentum_mode(momentum_input(3, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            momentum_mode(momentum_input(3, 4, 800, 2)),
            RustPredecodeMomentumMode::Neutral
        );
    }

    fn momentum_input(
        previous_page_index: i32,
        current_page_index: i32,
        elapsed_msec: i64,
        same_direction_move_count: i32,
    ) -> PredecodeMomentumInput {
        PredecodeMomentumInput {
            previous_page_index,
            current_page_index,
            elapsed_msec,
            same_direction_move_count,
        }
    }

    fn initial_momentum_state() -> RustPredecodeMomentumState {
        RustPredecodeMomentumState {
            last_page_index: -1,
            last_navigation_msec: -1,
            same_direction_move_count: 0,
            last_direction: RustPredecodeMomentumDirection::None,
            mode: RustPredecodeMomentumMode::Neutral,
        }
    }

    fn momentum_update_input(
        state: RustPredecodeMomentumState,
        page_index: i32,
        monotonic_msec: i64,
    ) -> RustPredecodeMomentumUpdateInput {
        RustPredecodeMomentumUpdateInput {
            state,
            page_index,
            monotonic_msec,
        }
    }
}
