// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const PREDECODE_PREFERRED_BYTE_BUDGET: i64 = 1024 * 1024 * 1024;
const PREDECODE_SYSTEM_MEMORY_DIVISOR: i64 = 8;
const REGULAR_NEUTRAL_PREVIOUS_IMAGE_COUNT: usize = 1;
const REGULAR_NEUTRAL_NEXT_IMAGE_COUNT: usize = 2;
const REGULAR_BIASED_DIRECTION_IMAGE_COUNT: usize = 3;
const DOCUMENT_NEUTRAL_PREVIOUS_IMAGE_COUNT: usize = 2;
const DOCUMENT_NEUTRAL_NEXT_IMAGE_COUNT: usize = 4;
const DIRECTORY_BIASED_DIRECTION_IMAGE_COUNT: usize = 6;
const ARCHIVE_BIASED_DIRECTION_IMAGE_COUNT: usize = 8;
const PREDECODE_BIASED_NAVIGATION_MSEC: i64 = 450;
const PREDECODE_SCRUBBING_NAVIGATION_MSEC: i64 = 120;
const PREDECODE_NEUTRAL_NAVIGATION_MSEC: i64 = 800;
const PREDECODE_SCRUBBING_PAGE_JUMP: i32 = 3;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustPredecodeDocumentKind {
        Regular = 0,
        DirectoryDocument = 1,
        ArchiveDocument = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustPredecodeMomentumMode {
        Neutral = 0,
        NextBiased = 1,
        PrevBiased = 2,
        ScrubbingNext = 3,
        ScrubbingPrev = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustPredecodeMomentumDirection {
        None = 0,
        Previous = 1,
        Next = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodePolicyInput {
        document_kind: RustPredecodeDocumentKind,
        momentum_mode: RustPredecodeMomentumMode,
        power_saver_enabled: bool,
        ideal_thread_count: i32,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustPredecodeSchedulePlan {
        parallel_limit: usize,
        target_indices: Vec<usize>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeMomentumState {
        last_page_index: i32,
        last_navigation_msec: i64,
        same_direction_move_count: i32,
        last_direction: RustPredecodeMomentumDirection,
        mode: RustPredecodeMomentumMode,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeMomentumUpdateInput {
        state: RustPredecodeMomentumState,
        page_index: i32,
        monotonic_msec: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadPlan {
        found: bool,
        index: usize,
        discard_count: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeCachedImageState {
        current_displayed: bool,
        recent_displayed: bool,
        current_displayed_priority: usize,
        recent_displayed_priority: usize,
        window_priority: usize,
        byte_cost: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeWindowLoadState {
        displayed: bool,
        cached: bool,
        in_flight: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadState {
        valid: bool,
        in_window: bool,
        cached: bool,
        in_flight: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodePreferredByteBudget"]
        fn rust_predecode_preferred_byte_budget() -> i64;

        #[cxx_name = "rustPredecodeDebounceMsec"]
        fn rust_predecode_debounce_msec() -> i32;

        #[cxx_name = "rustPredecodeNeutralRefreshMsec"]
        fn rust_predecode_neutral_refresh_msec() -> i32;

        #[cxx_name = "rustPredecodeByteBudgetForSystemMemory"]
        fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64;

        #[cxx_name = "rustPredecodeRetainedCachedImageIndices"]
        fn rust_predecode_retained_cached_image_indices(
            states: Vec<RustPredecodeCachedImageState>,
            window_count: usize,
            byte_budget: i64,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeMissingWindowLoadIndices"]
        fn rust_predecode_missing_window_load_indices(
            states: Vec<RustPredecodeWindowLoadState>,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeSchedulePlan"]
        fn rust_predecode_schedule_plan(
            candidate_count: usize,
            current_index_found: bool,
            current_index: usize,
            input: RustPredecodePolicyInput,
        ) -> RustPredecodeSchedulePlan;

        #[cxx_name = "rustPredecodeUpdatedMomentumState"]
        fn rust_predecode_updated_momentum_state(
            input: RustPredecodeMomentumUpdateInput,
        ) -> RustPredecodeMomentumState;

        #[cxx_name = "rustPredecodeNextQueuedLoadPlan"]
        fn rust_predecode_next_queued_load_plan(
            states: Vec<RustPredecodeQueuedLoadState>,
        ) -> RustPredecodeQueuedLoadPlan;
    }
}

use ffi::{
    RustPredecodeCachedImageState, RustPredecodeDocumentKind, RustPredecodeMomentumDirection,
    RustPredecodeMomentumMode, RustPredecodeMomentumState, RustPredecodeMomentumUpdateInput,
    RustPredecodePolicyInput, RustPredecodeQueuedLoadPlan, RustPredecodeQueuedLoadState,
    RustPredecodeSchedulePlan, RustPredecodeWindowLoadState,
};

#[derive(Clone, Copy)]
struct RetainedCachedImage {
    original_index: usize,
    current_displayed: bool,
    recent_displayed: bool,
    current_displayed_priority: usize,
    recent_displayed_priority: usize,
    window_priority: usize,
    byte_cost: i64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct PredecodeMomentumInput {
    previous_page_index: i32,
    current_page_index: i32,
    elapsed_msec: i64,
    same_direction_move_count: i32,
}

fn rust_predecode_preferred_byte_budget() -> i64 {
    PREDECODE_PREFERRED_BYTE_BUDGET
}

fn rust_predecode_debounce_msec() -> i32 {
    PREDECODE_SCRUBBING_NAVIGATION_MSEC
        .try_into()
        .unwrap_or(i32::MAX)
}

fn rust_predecode_neutral_refresh_msec() -> i32 {
    PREDECODE_NEUTRAL_NAVIGATION_MSEC
        .try_into()
        .unwrap_or(i32::MAX)
}

fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    crate::cachebudget::system_memory_capped_byte_budget(
        PREDECODE_PREFERRED_BYTE_BUDGET,
        system_memory_byte_size,
        PREDECODE_SYSTEM_MEMORY_DIVISOR,
    )
}

fn rust_predecode_retained_cached_image_indices(
    states: Vec<RustPredecodeCachedImageState>,
    window_count: usize,
    byte_budget: i64,
) -> Vec<usize> {
    if byte_budget <= 0 {
        return Vec::new();
    }

    let mut images: Vec<RetainedCachedImage> = states
        .into_iter()
        .enumerate()
        .filter_map(|(original_index, state)| {
            if (!state.current_displayed
                && !state.recent_displayed
                && state.window_priority >= window_count)
                || state.byte_cost <= 0
            {
                return None;
            }

            Some(RetainedCachedImage {
                original_index,
                current_displayed: state.current_displayed,
                recent_displayed: state.recent_displayed,
                current_displayed_priority: state.current_displayed_priority,
                recent_displayed_priority: state.recent_displayed_priority,
                window_priority: state.window_priority,
                byte_cost: state.byte_cost,
            })
        })
        .collect();

    let mut current_displayed_images: Vec<RetainedCachedImage> = images
        .iter()
        .copied()
        .filter(|image| image.current_displayed)
        .collect();
    current_displayed_images.sort_by(|left, right| {
        left.current_displayed_priority
            .cmp(&right.current_displayed_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let current_displayed_byte_cost = current_displayed_images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    let adjacent_byte_budget = byte_budget.saturating_sub(current_displayed_byte_cost);

    images.retain(|image| !image.current_displayed);
    images.sort_by(|left, right| {
        retention_group(*left)
            .cmp(&retention_group(*right))
            .then_with(|| retention_priority(*left).cmp(&retention_priority(*right)))
            .then(left.original_index.cmp(&right.original_index))
    });

    let mut retained_images = Vec::new();
    let mut retained_byte_cost = 0_i64;
    for image in images {
        if retained_byte_cost.saturating_add(image.byte_cost) > adjacent_byte_budget {
            break;
        }

        retained_byte_cost = retained_byte_cost.saturating_add(image.byte_cost);
        retained_images.push(image);
    }

    current_displayed_images.extend(retained_images);
    current_displayed_images
        .into_iter()
        .map(|image| image.original_index)
        .collect()
}

fn rust_predecode_missing_window_load_indices(
    states: Vec<RustPredecodeWindowLoadState>,
) -> Vec<usize> {
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

fn rust_predecode_schedule_plan(
    candidate_count: usize,
    current_index_found: bool,
    current_index: usize,
    input: RustPredecodePolicyInput,
) -> RustPredecodeSchedulePlan {
    let parallel_limit = predecode_parallel_limit(input);
    let target_indices = if parallel_limit == 0 {
        Vec::new()
    } else {
        predecode_target_image_indices(candidate_count, current_index_found, current_index, input)
    };

    RustPredecodeSchedulePlan {
        parallel_limit,
        target_indices,
    }
}

fn predecode_target_image_indices(
    candidate_count: usize,
    current_index_found: bool,
    current_index: usize,
    input: RustPredecodePolicyInput,
) -> Vec<usize> {
    let mut indices = Vec::new();
    if !current_index_found || current_index >= candidate_count {
        return indices;
    }

    indices.push(current_index);
    let profile = predecode_window_profile(input);
    append_directional_indices(
        &mut indices,
        current_index,
        candidate_count,
        profile.next_count,
        profile.previous_count,
    );
    indices
}

fn predecode_parallel_limit(input: RustPredecodePolicyInput) -> usize {
    if input.power_saver_enabled || scrubbing(input.momentum_mode) {
        return 0;
    }

    match input.document_kind {
        RustPredecodeDocumentKind::Regular => 1,
        RustPredecodeDocumentKind::DirectoryDocument => 2,
        RustPredecodeDocumentKind::ArchiveDocument => {
            let half_thread_count = input.ideal_thread_count / 2;
            usize::try_from(half_thread_count.clamp(1, 4)).unwrap_or(1)
        }
        _ => 1,
    }
}

#[derive(Clone, Copy)]
struct PredecodeWindowProfile {
    next_count: usize,
    previous_count: usize,
}

fn predecode_window_profile(input: RustPredecodePolicyInput) -> PredecodeWindowProfile {
    match input.momentum_mode {
        RustPredecodeMomentumMode::Neutral => neutral_window_profile(input.document_kind),
        RustPredecodeMomentumMode::NextBiased => biased_window_profile(input.document_kind, true),
        RustPredecodeMomentumMode::PrevBiased => biased_window_profile(input.document_kind, false),
        RustPredecodeMomentumMode::ScrubbingNext | RustPredecodeMomentumMode::ScrubbingPrev => {
            PredecodeWindowProfile {
                next_count: 0,
                previous_count: 0,
            }
        }
        _ => neutral_window_profile(input.document_kind),
    }
}

fn neutral_window_profile(document_kind: RustPredecodeDocumentKind) -> PredecodeWindowProfile {
    match document_kind {
        RustPredecodeDocumentKind::DirectoryDocument
        | RustPredecodeDocumentKind::ArchiveDocument => PredecodeWindowProfile {
            next_count: DOCUMENT_NEUTRAL_NEXT_IMAGE_COUNT,
            previous_count: DOCUMENT_NEUTRAL_PREVIOUS_IMAGE_COUNT,
        },
        RustPredecodeDocumentKind::Regular => PredecodeWindowProfile {
            next_count: REGULAR_NEUTRAL_NEXT_IMAGE_COUNT,
            previous_count: REGULAR_NEUTRAL_PREVIOUS_IMAGE_COUNT,
        },
        _ => PredecodeWindowProfile {
            next_count: REGULAR_NEUTRAL_NEXT_IMAGE_COUNT,
            previous_count: REGULAR_NEUTRAL_PREVIOUS_IMAGE_COUNT,
        },
    }
}

fn biased_window_profile(
    document_kind: RustPredecodeDocumentKind,
    next_biased: bool,
) -> PredecodeWindowProfile {
    let direction_count = match document_kind {
        RustPredecodeDocumentKind::ArchiveDocument => ARCHIVE_BIASED_DIRECTION_IMAGE_COUNT,
        RustPredecodeDocumentKind::DirectoryDocument => DIRECTORY_BIASED_DIRECTION_IMAGE_COUNT,
        RustPredecodeDocumentKind::Regular => REGULAR_BIASED_DIRECTION_IMAGE_COUNT,
        _ => REGULAR_BIASED_DIRECTION_IMAGE_COUNT,
    };
    if next_biased {
        return PredecodeWindowProfile {
            next_count: direction_count,
            previous_count: 1,
        };
    }

    PredecodeWindowProfile {
        next_count: 1,
        previous_count: direction_count,
    }
}

fn append_directional_indices(
    indices: &mut Vec<usize>,
    current_index: usize,
    candidate_count: usize,
    next_count: usize,
    previous_count: usize,
) {
    let append_offset = |indices: &mut Vec<usize>, offset: isize| {
        let Some(target_index) = current_index.checked_add_signed(offset) else {
            return;
        };
        if target_index >= candidate_count {
            return;
        }

        indices.push(target_index);
    };

    for offset in 1..=next_count.max(previous_count) {
        if offset <= next_count {
            append_offset(indices, offset as isize);
        }
        if offset <= previous_count {
            append_offset(indices, -(offset as isize));
        }
    }
}

fn scrubbing(mode: RustPredecodeMomentumMode) -> bool {
    matches!(
        mode,
        RustPredecodeMomentumMode::ScrubbingNext | RustPredecodeMomentumMode::ScrubbingPrev
    )
}

fn predecode_momentum_mode(input: PredecodeMomentumInput) -> RustPredecodeMomentumMode {
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

fn rust_predecode_updated_momentum_state(
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

    state.mode = predecode_momentum_mode(PredecodeMomentumInput {
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

fn rust_predecode_next_queued_load_plan(
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

fn retention_group(image: RetainedCachedImage) -> usize {
    if image.recent_displayed { 0 } else { 1 }
}

fn retention_priority(image: RetainedCachedImage) -> usize {
    if image.recent_displayed {
        image.recent_displayed_priority
    } else {
        image.window_priority
    }
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
    fn byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = rust_predecode_preferred_byte_budget();

        assert_eq!(rust_predecode_byte_budget_for_system_memory(0), preferred);
        assert_eq!(
            rust_predecode_byte_budget_for_system_memory(preferred),
            preferred / PREDECODE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            rust_predecode_byte_budget_for_system_memory(preferred * 16),
            preferred
        );
    }

    #[test]
    fn retained_cached_image_indices_drop_images_outside_window_and_sort_by_priority() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(2, 10),
                    cached_image_state(3, 10),
                    cached_image_state(0, 10),
                    cached_image_state(1, 10),
                ],
                3,
                100,
            ),
            vec![2, 3, 0]
        );
    }

    #[test]
    fn retained_cached_image_indices_trim_lowest_priority_images_to_budget() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(2, 50),
                    cached_image_state(0, 60),
                    cached_image_state(1, 60),
                ],
                3,
                120,
            ),
            vec![1, 2]
        );
    }

    #[test]
    fn retained_cached_image_indices_preserve_priority_prefix_when_trimming() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(0, 100),
                    cached_image_state(1, 100),
                    cached_image_state(2, 1),
                ],
                3,
                101,
            ),
            vec![0]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_before_adjacent_budget() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(0, 60),
                    displayed_cached_image_state(1, 90),
                    cached_image_state(2, 60),
                    displayed_cached_image_state(3, 90),
                ],
                4,
                160,
            ),
            vec![1, 3]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_without_window() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    displayed_cached_image_state(usize::MAX, 90),
                    cached_image_state(0, 10),
                ],
                0,
                80,
            ),
            vec![0]
        );
    }

    #[test]
    fn retained_cached_image_indices_prioritize_current_recent_then_window() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(0, 80),
                    recent_cached_image_state(0, 80),
                    displayed_cached_image_state(0, 80),
                ],
                1,
                160,
            ),
            vec![2, 1]
        );
    }

    #[test]
    fn missing_window_load_indices_skip_displayed_cached_and_in_flight_urls() {
        assert_eq!(
            rust_predecode_missing_window_load_indices(vec![
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
    fn schedule_plan_uses_regular_neutral_profile() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    RustPredecodeDocumentKind::Regular,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    8,
                ),
            ),
            schedule_plan(1, vec![5, 6, 4, 7])
        );
    }

    #[test]
    fn schedule_plan_uses_document_neutral_profile() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    8,
                ),
            ),
            schedule_plan(4, vec![5, 6, 4, 7, 3, 8, 9])
        );
    }

    #[test]
    fn schedule_plan_biases_paged_documents_in_navigation_direction() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::NextBiased,
                    false,
                    8,
                ),
            ),
            schedule_plan(4, vec![5, 6, 4, 7, 8, 9, 10, 11, 12, 13])
        );
        assert_eq!(
            schedule_plan_for_current(
                15,
                8,
                policy_input(
                    RustPredecodeDocumentKind::DirectoryDocument,
                    RustPredecodeMomentumMode::PrevBiased,
                    false,
                    8,
                ),
            ),
            schedule_plan(2, vec![8, 9, 7, 6, 5, 4, 3, 2])
        );
    }

    #[test]
    fn schedule_plan_suppresses_background_work_while_scrubbing_or_power_saver() {
        assert_eq!(
            schedule_plan_for_current(
                7,
                3,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::ScrubbingNext,
                    false,
                    8,
                ),
            ),
            schedule_plan(0, vec![])
        );
        assert_eq!(
            schedule_plan_for_current(
                7,
                3,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::NextBiased,
                    true,
                    8,
                ),
            ),
            schedule_plan(0, vec![])
        );
    }

    #[test]
    fn parallel_limit_uses_document_kind_and_runtime_state() {
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::Regular,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    8,
                ),
            )
            .parallel_limit,
            1,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::DirectoryDocument,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    8,
                ),
            )
            .parallel_limit,
            2,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    16,
                ),
            )
            .parallel_limit,
            4,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    1,
                ),
            )
            .parallel_limit,
            1,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::ScrubbingPrev,
                    false,
                    16,
                ),
            )
            .parallel_limit,
            0,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::Neutral,
                    true,
                    16,
                ),
            )
            .parallel_limit,
            0,
        );
    }

    #[test]
    fn momentum_mode_classifies_fast_repeated_and_large_page_moves() {
        assert_eq!(
            predecode_momentum_mode(momentum_input(4, 5, 300, 1)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(4, 5, 300, 2)),
            RustPredecodeMomentumMode::NextBiased
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(5, 4, 300, 2)),
            RustPredecodeMomentumMode::PrevBiased
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(4, 5, 120, 2)),
            RustPredecodeMomentumMode::ScrubbingNext
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(10, 6, 300, 1)),
            RustPredecodeMomentumMode::ScrubbingPrev
        );
    }

    #[test]
    fn updated_momentum_state_tracks_repeated_direction_in_rust() {
        let state = rust_predecode_updated_momentum_state(momentum_update_input(
            initial_momentum_state(),
            4,
            1000,
        ));
        assert_eq!(state.last_page_index, 4);
        assert_eq!(state.last_navigation_msec, 1000);
        assert_eq!(state.same_direction_move_count, 0);
        assert_eq!(state.last_direction, RustPredecodeMomentumDirection::None);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = rust_predecode_updated_momentum_state(momentum_update_input(state, 5, 1300));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(state.last_direction, RustPredecodeMomentumDirection::Next);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = rust_predecode_updated_momentum_state(momentum_update_input(state, 6, 1600));
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

        let state = rust_predecode_updated_momentum_state(momentum_update_input(state, 5, 1200));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(
            state.last_direction,
            RustPredecodeMomentumDirection::Previous
        );
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = rust_predecode_updated_momentum_state(momentum_update_input(state, 4, 2100));
        assert_eq!(state.same_direction_move_count, 1);
        assert_eq!(state.mode, RustPredecodeMomentumMode::Neutral);

        let state = rust_predecode_updated_momentum_state(momentum_update_input(state, -1, 2200));
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
            predecode_momentum_mode(momentum_input(-1, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(3, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            predecode_momentum_mode(momentum_input(3, 4, 800, 2)),
            RustPredecodeMomentumMode::Neutral
        );
    }

    #[test]
    fn schedule_plan_ignores_missing_current() {
        assert_eq!(
            schedule_plan_without_current(0, regular_policy_input()),
            schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_without_current(3, regular_policy_input()),
            schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_for_current(3, 3, regular_policy_input()),
            schedule_plan(1, vec![])
        );
    }

    #[test]
    fn next_queued_load_plan_skips_invalid_stale_and_cached_requests() {
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
                queued_load_state(true, true, true),
                queued_load_state(true, true, false),
            ]),
            queued_load_plan(3)
        );
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
            ]),
            missing_queued_load_plan(2)
        );
    }

    #[test]
    fn next_queued_load_plan_skips_in_flight_requests() {
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![
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

    fn cached_image_state(window_priority: usize, byte_cost: i64) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: false,
            recent_displayed: false,
            current_displayed_priority: usize::MAX,
            recent_displayed_priority: usize::MAX,
            window_priority,
            byte_cost,
        }
    }

    fn displayed_cached_image_state(
        window_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: true,
            recent_displayed: false,
            current_displayed_priority: window_priority,
            recent_displayed_priority: usize::MAX,
            window_priority,
            byte_cost,
        }
    }

    fn recent_cached_image_state(
        recent_displayed_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            current_displayed: false,
            recent_displayed: true,
            current_displayed_priority: usize::MAX,
            recent_displayed_priority,
            window_priority: usize::MAX,
            byte_cost,
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

    fn policy_input(
        document_kind: RustPredecodeDocumentKind,
        momentum_mode: RustPredecodeMomentumMode,
        power_saver_enabled: bool,
        ideal_thread_count: i32,
    ) -> RustPredecodePolicyInput {
        RustPredecodePolicyInput {
            document_kind,
            momentum_mode,
            power_saver_enabled,
            ideal_thread_count,
        }
    }

    fn regular_policy_input() -> RustPredecodePolicyInput {
        policy_input(
            RustPredecodeDocumentKind::Regular,
            RustPredecodeMomentumMode::Neutral,
            false,
            1,
        )
    }

    fn schedule_plan_for_current(
        candidate_count: usize,
        current_index: usize,
        input: RustPredecodePolicyInput,
    ) -> RustPredecodeSchedulePlan {
        rust_predecode_schedule_plan(candidate_count, true, current_index, input)
    }

    fn schedule_plan_without_current(
        candidate_count: usize,
        input: RustPredecodePolicyInput,
    ) -> RustPredecodeSchedulePlan {
        rust_predecode_schedule_plan(candidate_count, false, 0, input)
    }

    fn schedule_plan(
        parallel_limit: usize,
        target_indices: Vec<usize>,
    ) -> RustPredecodeSchedulePlan {
        RustPredecodeSchedulePlan {
            parallel_limit,
            target_indices,
        }
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
