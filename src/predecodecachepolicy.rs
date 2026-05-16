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
    struct RustPredecodePolicyInput {
        document_kind: RustPredecodeDocumentKind,
        momentum_mode: RustPredecodeMomentumMode,
        power_saver_enabled: bool,
        ideal_thread_count: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeMomentumInput {
        previous_page_index: i32,
        current_page_index: i32,
        elapsed_msec: i64,
        same_direction_move_count: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadPlan {
        found: bool,
        index: usize,
        discard_count: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeCachedImageState {
        displayed: bool,
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
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodePreferredByteBudget"]
        fn rust_predecode_preferred_byte_budget() -> i64;

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

        #[cxx_name = "rustPredecodeWindowImageIndices"]
        fn rust_predecode_window_image_indices(matches_current: Vec<u8>) -> Vec<usize>;

        #[cxx_name = "rustPredecodeTargetImageIndices"]
        fn rust_predecode_target_image_indices(
            matches_current: Vec<u8>,
            input: RustPredecodePolicyInput,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeParallelLimit"]
        fn rust_predecode_parallel_limit(input: RustPredecodePolicyInput) -> usize;

        #[cxx_name = "rustPredecodeMomentumMode"]
        fn rust_predecode_momentum_mode(
            input: RustPredecodeMomentumInput,
        ) -> RustPredecodeMomentumMode;

        #[cxx_name = "rustPredecodeNextQueuedLoadPlan"]
        fn rust_predecode_next_queued_load_plan(
            states: Vec<RustPredecodeQueuedLoadState>,
        ) -> RustPredecodeQueuedLoadPlan;
    }
}

use ffi::{
    RustPredecodeCachedImageState, RustPredecodeDocumentKind, RustPredecodeMomentumInput,
    RustPredecodeMomentumMode, RustPredecodePolicyInput, RustPredecodeQueuedLoadPlan,
    RustPredecodeQueuedLoadState, RustPredecodeWindowLoadState,
};

#[derive(Clone, Copy)]
struct RetainedCachedImage {
    original_index: usize,
    displayed: bool,
    window_priority: usize,
    byte_cost: i64,
}

fn rust_predecode_preferred_byte_budget() -> i64 {
    PREDECODE_PREFERRED_BYTE_BUDGET
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
            if (!state.displayed && state.window_priority >= window_count) || state.byte_cost <= 0 {
                return None;
            }

            Some(RetainedCachedImage {
                original_index,
                displayed: state.displayed,
                window_priority: state.window_priority,
                byte_cost: state.byte_cost,
            })
        })
        .collect();

    let mut displayed_images: Vec<RetainedCachedImage> = images
        .iter()
        .copied()
        .filter(|image| image.displayed)
        .collect();
    displayed_images.sort_by(|left, right| {
        left.window_priority
            .cmp(&right.window_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let displayed_byte_cost = displayed_images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    let adjacent_byte_budget = byte_budget.saturating_sub(displayed_byte_cost);

    images.retain(|image| !image.displayed);
    images.sort_by(|left, right| {
        left.window_priority
            .cmp(&right.window_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let mut total_byte_cost = images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    while total_byte_cost > adjacent_byte_budget && !images.is_empty() {
        if let Some(image) = images.pop() {
            total_byte_cost = total_byte_cost.saturating_sub(image.byte_cost);
        }
    }

    displayed_images.extend(images);
    displayed_images
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

fn rust_predecode_window_image_indices(matches_current: Vec<u8>) -> Vec<usize> {
    rust_predecode_target_image_indices(matches_current, default_predecode_policy_input())
}

fn rust_predecode_target_image_indices(
    matches_current: Vec<u8>,
    input: RustPredecodePolicyInput,
) -> Vec<usize> {
    let mut indices = Vec::new();
    let candidate_count = matches_current.len();
    let Some(current_index) = matches_current.iter().position(|flag| *flag != 0) else {
        return indices;
    };

    indices.push(current_index);
    if input.power_saver_enabled || scrubbing(input.momentum_mode) {
        return indices;
    }

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

fn rust_predecode_parallel_limit(input: RustPredecodePolicyInput) -> usize {
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

fn default_predecode_policy_input() -> RustPredecodePolicyInput {
    RustPredecodePolicyInput {
        document_kind: RustPredecodeDocumentKind::Regular,
        momentum_mode: RustPredecodeMomentumMode::Neutral,
        power_saver_enabled: false,
        ideal_thread_count: 1,
    }
}

fn rust_predecode_momentum_mode(input: RustPredecodeMomentumInput) -> RustPredecodeMomentumMode {
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

fn rust_predecode_next_queued_load_plan(
    states: Vec<RustPredecodeQueuedLoadState>,
) -> RustPredecodeQueuedLoadPlan {
    let state_count = states.len();
    for (index, state) in states.into_iter().enumerate() {
        if state.valid && state.in_window && !state.cached {
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
    fn target_image_indices_use_regular_neutral_profile() {
        assert_eq!(
            rust_predecode_window_image_indices(vec![0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
            vec![5, 6, 4, 7]
        );
    }

    #[test]
    fn target_image_indices_use_document_neutral_profile() {
        assert_eq!(
            rust_predecode_target_image_indices(
                vec![0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                    8,
                ),
            ),
            vec![5, 6, 4, 7, 3, 8, 9]
        );
    }

    #[test]
    fn target_image_indices_bias_paged_documents_in_navigation_direction() {
        assert_eq!(
            rust_predecode_target_image_indices(
                vec![0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::NextBiased,
                    false,
                    8,
                ),
            ),
            vec![5, 6, 4, 7, 8, 9, 10, 11, 12, 13]
        );
        assert_eq!(
            rust_predecode_target_image_indices(
                vec![0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0],
                policy_input(
                    RustPredecodeDocumentKind::DirectoryDocument,
                    RustPredecodeMomentumMode::PrevBiased,
                    false,
                    8,
                ),
            ),
            vec![8, 9, 7, 6, 5, 4, 3, 2]
        );
    }

    #[test]
    fn target_image_indices_keep_only_current_while_scrubbing_or_power_saver() {
        assert_eq!(
            rust_predecode_target_image_indices(
                vec![0, 0, 0, 1, 0, 0, 0],
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::ScrubbingNext,
                    false,
                    8,
                ),
            ),
            vec![3]
        );
        assert_eq!(
            rust_predecode_target_image_indices(
                vec![0, 0, 0, 1, 0, 0, 0],
                policy_input(
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::NextBiased,
                    true,
                    8,
                ),
            ),
            vec![3]
        );
    }

    #[test]
    fn parallel_limit_uses_document_kind_and_runtime_state() {
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::Regular,
                RustPredecodeMomentumMode::Neutral,
                false,
                8,
            )),
            1
        );
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::DirectoryDocument,
                RustPredecodeMomentumMode::Neutral,
                false,
                8,
            )),
            2
        );
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::ArchiveDocument,
                RustPredecodeMomentumMode::Neutral,
                false,
                16,
            )),
            4
        );
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::ArchiveDocument,
                RustPredecodeMomentumMode::Neutral,
                false,
                1,
            )),
            1
        );
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::ArchiveDocument,
                RustPredecodeMomentumMode::ScrubbingPrev,
                false,
                16,
            )),
            0
        );
        assert_eq!(
            rust_predecode_parallel_limit(policy_input(
                RustPredecodeDocumentKind::ArchiveDocument,
                RustPredecodeMomentumMode::Neutral,
                true,
                16,
            )),
            0
        );
    }

    #[test]
    fn momentum_mode_classifies_fast_repeated_and_large_page_moves() {
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(4, 5, 300, 1)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(4, 5, 300, 2)),
            RustPredecodeMomentumMode::NextBiased
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(5, 4, 300, 2)),
            RustPredecodeMomentumMode::PrevBiased
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(4, 5, 120, 2)),
            RustPredecodeMomentumMode::ScrubbingNext
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(10, 6, 300, 1)),
            RustPredecodeMomentumMode::ScrubbingPrev
        );
    }

    #[test]
    fn momentum_mode_returns_neutral_for_invalid_or_settled_navigation() {
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(-1, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(3, 3, 100, 2)),
            RustPredecodeMomentumMode::Neutral
        );
        assert_eq!(
            rust_predecode_momentum_mode(momentum_input(3, 4, 800, 2)),
            RustPredecodeMomentumMode::Neutral
        );
    }

    #[test]
    fn window_image_indices_ignore_missing_current() {
        assert_eq!(
            rust_predecode_window_image_indices(vec![]),
            Vec::<usize>::new()
        );
        assert_eq!(
            rust_predecode_window_image_indices(vec![0, 0, 0]),
            Vec::<usize>::new()
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
            displayed: false,
            window_priority,
            byte_cost,
        }
    }

    fn displayed_cached_image_state(
        window_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            displayed: true,
            window_priority,
            byte_cost,
        }
    }

    fn queued_load_state(
        valid: bool,
        in_window: bool,
        cached: bool,
    ) -> RustPredecodeQueuedLoadState {
        RustPredecodeQueuedLoadState {
            valid,
            in_window,
            cached,
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

    fn momentum_input(
        previous_page_index: i32,
        current_page_index: i32,
        elapsed_msec: i64,
        same_direction_move_count: i32,
    ) -> RustPredecodeMomentumInput {
        RustPredecodeMomentumInput {
            previous_page_index,
            current_page_index,
            elapsed_msec,
            same_direction_move_count,
        }
    }
}
