// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::{
    RustPredecodeMomentumMode, RustPredecodePolicyInput, RustPredecodeSchedulePlan,
    RustPredecodeSourceProfile,
};

#[derive(Clone, Copy)]
struct PredecodeWindowProfile {
    next_count: usize,
    previous_count: usize,
}

pub(super) fn schedule_plan(
    candidate_count: usize,
    current_index_found: bool,
    current_index: usize,
    input: RustPredecodePolicyInput,
) -> RustPredecodeSchedulePlan {
    let parallel_limit = parallel_limit(input);
    let target_indices = if parallel_limit == 0 {
        Vec::new()
    } else {
        target_image_indices(candidate_count, current_index_found, current_index, input)
    };

    RustPredecodeSchedulePlan {
        parallel_limit,
        target_indices,
    }
}

fn target_image_indices(
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
    let profile = window_profile(input);
    append_directional_indices(
        &mut indices,
        current_index,
        candidate_count,
        profile.next_count,
        profile.previous_count,
    );
    indices
}

fn parallel_limit(input: RustPredecodePolicyInput) -> usize {
    if input.power_saver_enabled || scrubbing(input.momentum_mode) {
        return 0;
    }

    input.source_profile.parallel_limit
}

fn window_profile(input: RustPredecodePolicyInput) -> PredecodeWindowProfile {
    match input.momentum_mode {
        RustPredecodeMomentumMode::Neutral => neutral_window_profile(input.source_profile),
        RustPredecodeMomentumMode::NextBiased => biased_window_profile(input.source_profile, true),
        RustPredecodeMomentumMode::PrevBiased => biased_window_profile(input.source_profile, false),
        RustPredecodeMomentumMode::ScrubbingNext | RustPredecodeMomentumMode::ScrubbingPrev => {
            PredecodeWindowProfile {
                next_count: 0,
                previous_count: 0,
            }
        }
        _ => neutral_window_profile(input.source_profile),
    }
}

fn neutral_window_profile(source_profile: RustPredecodeSourceProfile) -> PredecodeWindowProfile {
    PredecodeWindowProfile {
        next_count: source_profile.neutral_next_image_count,
        previous_count: source_profile.neutral_previous_image_count,
    }
}

fn biased_window_profile(
    source_profile: RustPredecodeSourceProfile,
    next_biased: bool,
) -> PredecodeWindowProfile {
    let direction_count = source_profile.biased_direction_image_count;
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

#[cfg(test)]
mod tests {
    use super::*;

    const DIRECT_MEDIA_PROFILE: RustPredecodeSourceProfile = RustPredecodeSourceProfile {
        neutral_previous_image_count: 1,
        neutral_next_image_count: 2,
        biased_direction_image_count: 3,
        parallel_limit: 1,
    };
    const DIRECTORY_COLLECTION_PROFILE: RustPredecodeSourceProfile = RustPredecodeSourceProfile {
        neutral_previous_image_count: 2,
        neutral_next_image_count: 4,
        biased_direction_image_count: 6,
        parallel_limit: 2,
    };
    const ARCHIVE_COLLECTION_PROFILE: RustPredecodeSourceProfile = RustPredecodeSourceProfile {
        neutral_previous_image_count: 2,
        neutral_next_image_count: 4,
        biased_direction_image_count: 8,
        parallel_limit: 4,
    };

    #[test]
    fn schedule_plan_uses_direct_media_neutral_profile() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    DIRECT_MEDIA_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            ),
            expected_schedule_plan(1, vec![5, 6, 4, 7])
        );
    }

    #[test]
    fn schedule_plan_uses_collection_neutral_profile() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            ),
            expected_schedule_plan(4, vec![5, 6, 4, 7, 3, 8, 9])
        );
    }

    #[test]
    fn schedule_plan_biases_opened_collections_in_navigation_direction() {
        assert_eq!(
            schedule_plan_for_current(
                15,
                5,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::NextBiased,
                    false,
                ),
            ),
            expected_schedule_plan(4, vec![5, 6, 4, 7, 8, 9, 10, 11, 12, 13])
        );
        assert_eq!(
            schedule_plan_for_current(
                15,
                8,
                policy_input(
                    DIRECTORY_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::PrevBiased,
                    false,
                ),
            ),
            expected_schedule_plan(2, vec![8, 9, 7, 6, 5, 4, 3, 2])
        );
    }

    #[test]
    fn schedule_plan_suppresses_background_work_while_scrubbing_or_power_saver() {
        assert_eq!(
            schedule_plan_for_current(
                7,
                3,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::ScrubbingNext,
                    false,
                ),
            ),
            expected_schedule_plan(0, vec![])
        );
        assert_eq!(
            schedule_plan_for_current(
                7,
                3,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::NextBiased,
                    true,
                ),
            ),
            expected_schedule_plan(0, vec![])
        );
    }

    #[test]
    fn parallel_limit_uses_source_profile_and_runtime_state() {
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    DIRECT_MEDIA_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            )
            .parallel_limit,
            1,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    DIRECTORY_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            )
            .parallel_limit,
            2,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            )
            .parallel_limit,
            4,
        );
        let low_parallel_archive_profile = RustPredecodeSourceProfile {
            parallel_limit: 1,
            ..ARCHIVE_COLLECTION_PROFILE
        };
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    low_parallel_archive_profile,
                    RustPredecodeMomentumMode::Neutral,
                    false,
                ),
            )
            .parallel_limit,
            1,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::ScrubbingPrev,
                    false,
                ),
            )
            .parallel_limit,
            0,
        );
        assert_eq!(
            schedule_plan_without_current(
                0,
                policy_input(
                    ARCHIVE_COLLECTION_PROFILE,
                    RustPredecodeMomentumMode::Neutral,
                    true,
                ),
            )
            .parallel_limit,
            0,
        );
    }

    #[test]
    fn schedule_plan_ignores_missing_current() {
        assert_eq!(
            schedule_plan_without_current(0, direct_media_policy_input()),
            expected_schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_without_current(3, direct_media_policy_input()),
            expected_schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_for_current(3, 3, direct_media_policy_input()),
            expected_schedule_plan(1, vec![])
        );
    }

    fn policy_input(
        source_profile: RustPredecodeSourceProfile,
        momentum_mode: RustPredecodeMomentumMode,
        power_saver_enabled: bool,
    ) -> RustPredecodePolicyInput {
        RustPredecodePolicyInput {
            source_profile,
            momentum_mode,
            power_saver_enabled,
        }
    }

    fn direct_media_policy_input() -> RustPredecodePolicyInput {
        policy_input(
            DIRECT_MEDIA_PROFILE,
            RustPredecodeMomentumMode::Neutral,
            false,
        )
    }

    fn schedule_plan_for_current(
        candidate_count: usize,
        current_index: usize,
        input: RustPredecodePolicyInput,
    ) -> RustPredecodeSchedulePlan {
        schedule_plan(candidate_count, true, current_index, input)
    }

    fn schedule_plan_without_current(
        candidate_count: usize,
        input: RustPredecodePolicyInput,
    ) -> RustPredecodeSchedulePlan {
        schedule_plan(candidate_count, false, 0, input)
    }

    fn expected_schedule_plan(
        parallel_limit: usize,
        target_indices: Vec<usize>,
    ) -> RustPredecodeSchedulePlan {
        RustPredecodeSchedulePlan {
            parallel_limit,
            target_indices,
        }
    }
}
