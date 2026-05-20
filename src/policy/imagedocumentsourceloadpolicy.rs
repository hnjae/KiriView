// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadKind {
        CurrentSource = 0,
        ReplacementSource = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadOperation {
        CancelFileDeletion = 0,
        CancelAllNavigation = 1,
        CancelPredecode = 2,
        FinishSpreadTransition = 3,
        ResetRightToLeftReading = 4,
        NotifyRightToLeftReadingChanged = 5,
        ClearSecondaryPage = 6,
        ClearLoadingContainerNavigationUrl = 7,
        SetLoadingContainerNavigationUrlToRequested = 8,
        SetContainerNavigationUrlToRequested = 9,
        PrepareSourceLoad = 10,
        SetSourceUrlToRequested = 11,
        BeginOpen = 12,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPolicyInput {
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    }

    #[derive(Clone, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPlan {
        operations: Vec<RustImageDocumentSourceLoadOperation>,
    }

    extern "Rust" {
        #[cxx_name = "rustImageDocumentSourceLoadPlan"]
        fn rust_image_document_source_load_plan(
            input: RustImageDocumentSourceLoadPolicyInput,
        ) -> RustImageDocumentSourceLoadPlan;
    }
}

use ffi::{
    RustImageDocumentSourceLoadKind, RustImageDocumentSourceLoadOperation,
    RustImageDocumentSourceLoadPlan, RustImageDocumentSourceLoadPolicyInput,
};

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    match input.load_kind {
        RustImageDocumentSourceLoadKind::CurrentSource => current_source_load_plan(input),
        RustImageDocumentSourceLoadKind::ReplacementSource => replacement_source_load_plan(input),
        _ => current_source_load_plan(input),
    }
}

fn current_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = source_load_plan();
    push_if(
        &mut plan,
        !input.preserve_two_page_spread_transition,
        RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
    );
    apply_right_to_left_reading_transition(&mut plan, input, false);
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
    );
    if input.has_requested_container_navigation_url {
        push_operation(
            &mut plan,
            RustImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested,
        );
    }
    plan
}

fn replacement_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = source_load_plan();
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::CancelAllNavigation,
    );
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::CancelPredecode,
    );
    push_if(
        &mut plan,
        !input.preserve_two_page_spread_transition,
        RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
    );
    apply_right_to_left_reading_transition(&mut plan, input, true);
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
    );
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
    );
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
    );
    push_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
    );
    push_operation(&mut plan, RustImageDocumentSourceLoadOperation::BeginOpen);
    if right_to_left_reading_reset_applies(input) {
        push_operation(
            &mut plan,
            RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
        );
    }
    plan
}

fn apply_right_to_left_reading_transition(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
    replacement_source: bool,
) {
    if !right_to_left_reading_reset_applies(input) {
        return;
    }

    push_operation(
        plan,
        RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
    );
    if !replacement_source {
        push_operation(
            plan,
            RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
        );
    }
}

fn right_to_left_reading_reset_applies(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.right_to_left_reading_enabled
        && !input.has_requested_container_navigation_url
        && !input.source_within_displayed_comic_book_archive
}

fn push_if(
    plan: &mut RustImageDocumentSourceLoadPlan,
    condition: bool,
    operation: RustImageDocumentSourceLoadOperation,
) {
    if condition {
        push_operation(plan, operation);
    }
}

fn push_operation(
    plan: &mut RustImageDocumentSourceLoadPlan,
    operation: RustImageDocumentSourceLoadOperation,
) {
    plan.operations.push(operation);
}

fn source_load_plan() -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        operations: vec![RustImageDocumentSourceLoadOperation::CancelFileDeletion],
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn current_source_load_plan_uses_reading_and_container_snapshots() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
                ],
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
                    RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
                    RustImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested,
                ],
            }
        );
    }

    #[test]
    fn replacement_source_load_plan_begins_open_and_clears_competing_state_in_order() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                    RustImageDocumentSourceLoadOperation::CancelPredecode,
                    RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                    RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                    RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                    RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                    RustImageDocumentSourceLoadOperation::BeginOpen,
                ],
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                    RustImageDocumentSourceLoadOperation::CancelPredecode,
                    RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
                    RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                    RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                    RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                    RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                    RustImageDocumentSourceLoadOperation::BeginOpen,
                ],
            }
        );
    }

    #[test]
    fn right_to_left_reading_reset_is_decided_from_source_load_context() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                true,
                false,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
                RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
                RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
                RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
                RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                true,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                true,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
            ]
        );
    }

    fn source_load_policy_input(
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    ) -> RustImageDocumentSourceLoadPolicyInput {
        RustImageDocumentSourceLoadPolicyInput {
            load_kind,
            preserve_two_page_spread_transition,
            right_to_left_reading_enabled,
            source_within_displayed_comic_book_archive,
            has_requested_container_navigation_url,
        }
    }
}
