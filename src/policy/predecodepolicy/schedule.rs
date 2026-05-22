// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::{
    RustPredecodeDocumentKind, RustPredecodeMomentumMode, RustPredecodePolicyInput,
    RustPredecodeSchedulePlan,
};

const REGULAR_NEUTRAL_PREVIOUS_IMAGE_COUNT: usize = 1;
const REGULAR_NEUTRAL_NEXT_IMAGE_COUNT: usize = 2;
const REGULAR_BIASED_DIRECTION_IMAGE_COUNT: usize = 3;
const DOCUMENT_NEUTRAL_PREVIOUS_IMAGE_COUNT: usize = 2;
const DOCUMENT_NEUTRAL_NEXT_IMAGE_COUNT: usize = 4;
const DIRECTORY_BIASED_DIRECTION_IMAGE_COUNT: usize = 6;
const ARCHIVE_BIASED_DIRECTION_IMAGE_COUNT: usize = 8;

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

fn window_profile(input: RustPredecodePolicyInput) -> PredecodeWindowProfile {
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

#[cfg(test)]
mod tests {
    use super::*;

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
            expected_schedule_plan(1, vec![5, 6, 4, 7])
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
            expected_schedule_plan(4, vec![5, 6, 4, 7, 3, 8, 9])
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
            expected_schedule_plan(4, vec![5, 6, 4, 7, 8, 9, 10, 11, 12, 13])
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
                    RustPredecodeDocumentKind::ArchiveDocument,
                    RustPredecodeMomentumMode::ScrubbingNext,
                    false,
                    8,
                ),
            ),
            expected_schedule_plan(0, vec![])
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
            expected_schedule_plan(0, vec![])
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
    fn schedule_plan_ignores_missing_current() {
        assert_eq!(
            schedule_plan_without_current(0, regular_policy_input()),
            expected_schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_without_current(3, regular_policy_input()),
            expected_schedule_plan(1, vec![])
        );
        assert_eq!(
            schedule_plan_for_current(3, 3, regular_policy_input()),
            expected_schedule_plan(1, vec![])
        );
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
