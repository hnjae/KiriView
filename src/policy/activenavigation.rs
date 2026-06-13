// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "kiriview")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustActiveNavigationSourceKind {
        NoSource = 0,
        OrdinaryDirectMedia = 1,
        ImageDocumentPages = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustActiveNavigationBoundaryScope {
        NoBoundary = 0,
        DirectMedia = 1,
        ImageDocumentPage = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustActiveNavigationDispatchRequestKind {
        Previous = 0,
        Next = 1,
        First = 2,
        Last = 3,
        Number = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustActiveNavigationDispatchOutcome {
        NoOp = 0,
        Dispatch = 1,
        FirstBoundary = 2,
        LastBoundary = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustActiveNavigationDispatchOperationKind {
        NoOperation = 0,
        OpenPreviousDirectMedia = 1,
        OpenNextDirectMedia = 2,
        OpenDirectMediaAtNumber = 3,
        OpenPreviousImageDocumentPage = 4,
        OpenNextImageDocumentPage = 5,
        OpenImageDocumentPageAtNumber = 6,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDirectMediaNavigationBoundaryState {
        can_open_previous: bool,
        can_open_next: bool,
        at_known_first: bool,
        at_known_last: bool,
        current_number: i32,
        count: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDirectMediaActiveNavigationInput {
        boundary_state: RustDirectMediaNavigationBoundaryState,
        known: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentPageActiveNavigationSnapshot {
        known: bool,
        can_open_previous: bool,
        can_open_next: bool,
        at_known_first: bool,
        at_known_last: bool,
        current_number: i32,
        count: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustActiveNavigationSnapshot {
        available: bool,
        known: bool,
        editable: bool,
        can_open_previous: bool,
        can_open_next: bool,
        at_known_first: bool,
        at_known_last: bool,
        current_number: i32,
        count: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustActiveNavigationDispatchRequest {
        kind: RustActiveNavigationDispatchRequestKind,
        number: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustActiveNavigationDispatchPlan {
        operation_kind: RustActiveNavigationDispatchOperationKind,
        operation_number: i32,
        outcome: RustActiveNavigationDispatchOutcome,
    }

    extern "Rust" {
        #[cxx_name = "rustProjectActiveNavigation"]
        fn rust_project_active_navigation(
            source_kind: RustActiveNavigationSourceKind,
            direct_media_input: RustDirectMediaActiveNavigationInput,
            image_document_page_snapshot: RustImageDocumentPageActiveNavigationSnapshot,
            file_deletion_in_progress: bool,
        ) -> RustActiveNavigationSnapshot;

        #[cxx_name = "rustMaskActiveNavigationDuringDeletion"]
        fn rust_mask_active_navigation_during_deletion(
            snapshot: RustActiveNavigationSnapshot,
        ) -> RustActiveNavigationSnapshot;

        #[cxx_name = "rustActiveNavigationBoundaryScopeForSource"]
        fn rust_active_navigation_boundary_scope_for_source(
            source_kind: RustActiveNavigationSourceKind,
        ) -> RustActiveNavigationBoundaryScope;

        #[cxx_name = "rustActiveNavigationDispatchPlan"]
        fn rust_active_navigation_dispatch_plan(
            source_kind: RustActiveNavigationSourceKind,
            snapshot: RustActiveNavigationSnapshot,
            request: RustActiveNavigationDispatchRequest,
        ) -> RustActiveNavigationDispatchPlan;
    }
}

use ffi::{
    RustActiveNavigationBoundaryScope, RustActiveNavigationDispatchOperationKind,
    RustActiveNavigationDispatchOutcome, RustActiveNavigationDispatchPlan,
    RustActiveNavigationDispatchRequest, RustActiveNavigationDispatchRequestKind,
    RustActiveNavigationSnapshot, RustActiveNavigationSourceKind,
    RustDirectMediaActiveNavigationInput, RustImageDocumentPageActiveNavigationSnapshot,
};

fn unavailable_active_navigation() -> RustActiveNavigationSnapshot {
    active_navigation_snapshot(false, false, false, false, false, false, false, 0, 0)
}

fn unknown_active_navigation() -> RustActiveNavigationSnapshot {
    active_navigation_snapshot(true, false, false, false, false, false, false, 0, 0)
}

fn normalized_active_navigation(
    mut snapshot: RustActiveNavigationSnapshot,
) -> RustActiveNavigationSnapshot {
    if !snapshot.available
        || !snapshot.known
        || snapshot.current_number < 1
        || snapshot.count < 1
        || snapshot.current_number > snapshot.count
    {
        return if snapshot.available {
            unknown_active_navigation()
        } else {
            unavailable_active_navigation()
        };
    }

    snapshot.known = true;
    snapshot.editable = true;
    snapshot
}

fn direct_media_active_navigation_snapshot(
    input: RustDirectMediaActiveNavigationInput,
) -> RustActiveNavigationSnapshot {
    if !input.known {
        return unknown_active_navigation();
    }

    normalized_active_navigation(active_navigation_snapshot(
        true,
        true,
        true,
        input.boundary_state.can_open_previous,
        input.boundary_state.can_open_next,
        input.boundary_state.at_known_first,
        input.boundary_state.at_known_last,
        input.boundary_state.current_number,
        input.boundary_state.count,
    ))
}

fn image_document_page_active_navigation_snapshot(
    input: RustImageDocumentPageActiveNavigationSnapshot,
) -> RustActiveNavigationSnapshot {
    normalized_active_navigation(active_navigation_snapshot(
        true,
        input.known,
        true,
        input.can_open_previous,
        input.can_open_next,
        input.at_known_first,
        input.at_known_last,
        input.current_number,
        input.count,
    ))
}

fn rust_project_active_navigation(
    source_kind: RustActiveNavigationSourceKind,
    direct_media_input: RustDirectMediaActiveNavigationInput,
    image_document_page_snapshot: RustImageDocumentPageActiveNavigationSnapshot,
    file_deletion_in_progress: bool,
) -> RustActiveNavigationSnapshot {
    let snapshot = match source_kind {
        RustActiveNavigationSourceKind::OrdinaryDirectMedia => {
            direct_media_active_navigation_snapshot(direct_media_input)
        }
        RustActiveNavigationSourceKind::ImageDocumentPages => {
            image_document_page_active_navigation_snapshot(image_document_page_snapshot)
        }
        RustActiveNavigationSourceKind::NoSource => unavailable_active_navigation(),
        _ => unavailable_active_navigation(),
    };

    if file_deletion_in_progress {
        rust_mask_active_navigation_during_deletion(snapshot)
    } else {
        snapshot
    }
}

fn rust_mask_active_navigation_during_deletion(
    mut snapshot: RustActiveNavigationSnapshot,
) -> RustActiveNavigationSnapshot {
    snapshot.editable = false;
    snapshot.can_open_previous = false;
    snapshot.can_open_next = false;
    snapshot
}

fn rust_active_navigation_boundary_scope_for_source(
    source_kind: RustActiveNavigationSourceKind,
) -> RustActiveNavigationBoundaryScope {
    match source_kind {
        RustActiveNavigationSourceKind::OrdinaryDirectMedia => {
            RustActiveNavigationBoundaryScope::DirectMedia
        }
        RustActiveNavigationSourceKind::ImageDocumentPages => {
            RustActiveNavigationBoundaryScope::ImageDocumentPage
        }
        RustActiveNavigationSourceKind::NoSource => RustActiveNavigationBoundaryScope::NoBoundary,
        _ => RustActiveNavigationBoundaryScope::NoBoundary,
    }
}

fn rust_active_navigation_dispatch_plan(
    source_kind: RustActiveNavigationSourceKind,
    snapshot: RustActiveNavigationSnapshot,
    request: RustActiveNavigationDispatchRequest,
) -> RustActiveNavigationDispatchPlan {
    if !snapshot.available {
        return no_op_plan();
    }

    match request.kind {
        RustActiveNavigationDispatchRequestKind::Previous => {
            if snapshot.can_open_previous {
                return dispatch_operation(previous_operation_for_source(source_kind), 0);
            }
            if snapshot.known && snapshot.editable && snapshot.at_known_first {
                return boundary_outcome(RustActiveNavigationDispatchOutcome::FirstBoundary);
            }
            no_op_plan()
        }
        RustActiveNavigationDispatchRequestKind::Next => {
            if snapshot.can_open_next {
                return dispatch_operation(next_operation_for_source(source_kind), 0);
            }
            if snapshot.known && snapshot.editable && snapshot.at_known_last {
                return boundary_outcome(RustActiveNavigationDispatchOutcome::LastBoundary);
            }
            no_op_plan()
        }
        RustActiveNavigationDispatchRequestKind::First => {
            if snapshot.known && snapshot.editable && !snapshot.at_known_first {
                if let Some(number) = valid_dispatch_number(snapshot, 1) {
                    dispatch_operation(numbered_operation_for_source(source_kind), number)
                } else {
                    no_op_plan()
                }
            } else {
                no_op_plan()
            }
        }
        RustActiveNavigationDispatchRequestKind::Last => {
            if snapshot.known && snapshot.editable && !snapshot.at_known_last {
                if let Some(number) = valid_dispatch_number(snapshot, snapshot.count) {
                    dispatch_operation(numbered_operation_for_source(source_kind), number)
                } else {
                    no_op_plan()
                }
            } else {
                no_op_plan()
            }
        }
        RustActiveNavigationDispatchRequestKind::Number => {
            if let Some(number) = valid_dispatch_number(snapshot, request.number) {
                dispatch_operation(numbered_operation_for_source(source_kind), number)
            } else {
                no_op_plan()
            }
        }
        _ => no_op_plan(),
    }
}

fn valid_dispatch_number(snapshot: RustActiveNavigationSnapshot, number: i32) -> Option<i32> {
    if !snapshot.known
        || !snapshot.editable
        || snapshot.current_number < 1
        || snapshot.count < 1
        || snapshot.current_number > snapshot.count
    {
        return None;
    }

    if number <= 1 {
        return Some(1);
    }

    Some(number.min(snapshot.count))
}

fn previous_operation_for_source(
    source_kind: RustActiveNavigationSourceKind,
) -> RustActiveNavigationDispatchOperationKind {
    match source_kind {
        RustActiveNavigationSourceKind::OrdinaryDirectMedia => {
            RustActiveNavigationDispatchOperationKind::OpenPreviousDirectMedia
        }
        RustActiveNavigationSourceKind::ImageDocumentPages => {
            RustActiveNavigationDispatchOperationKind::OpenPreviousImageDocumentPage
        }
        RustActiveNavigationSourceKind::NoSource => {
            RustActiveNavigationDispatchOperationKind::NoOperation
        }
        _ => RustActiveNavigationDispatchOperationKind::NoOperation,
    }
}

fn next_operation_for_source(
    source_kind: RustActiveNavigationSourceKind,
) -> RustActiveNavigationDispatchOperationKind {
    match source_kind {
        RustActiveNavigationSourceKind::OrdinaryDirectMedia => {
            RustActiveNavigationDispatchOperationKind::OpenNextDirectMedia
        }
        RustActiveNavigationSourceKind::ImageDocumentPages => {
            RustActiveNavigationDispatchOperationKind::OpenNextImageDocumentPage
        }
        RustActiveNavigationSourceKind::NoSource => {
            RustActiveNavigationDispatchOperationKind::NoOperation
        }
        _ => RustActiveNavigationDispatchOperationKind::NoOperation,
    }
}

fn numbered_operation_for_source(
    source_kind: RustActiveNavigationSourceKind,
) -> RustActiveNavigationDispatchOperationKind {
    match source_kind {
        RustActiveNavigationSourceKind::OrdinaryDirectMedia => {
            RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber
        }
        RustActiveNavigationSourceKind::ImageDocumentPages => {
            RustActiveNavigationDispatchOperationKind::OpenImageDocumentPageAtNumber
        }
        RustActiveNavigationSourceKind::NoSource => {
            RustActiveNavigationDispatchOperationKind::NoOperation
        }
        _ => RustActiveNavigationDispatchOperationKind::NoOperation,
    }
}

fn dispatch_operation(
    operation_kind: RustActiveNavigationDispatchOperationKind,
    operation_number: i32,
) -> RustActiveNavigationDispatchPlan {
    if operation_kind == RustActiveNavigationDispatchOperationKind::NoOperation {
        return no_op_plan();
    }

    RustActiveNavigationDispatchPlan {
        operation_kind,
        operation_number,
        outcome: RustActiveNavigationDispatchOutcome::Dispatch,
    }
}

fn boundary_outcome(
    outcome: RustActiveNavigationDispatchOutcome,
) -> RustActiveNavigationDispatchPlan {
    RustActiveNavigationDispatchPlan {
        operation_kind: RustActiveNavigationDispatchOperationKind::NoOperation,
        operation_number: 0,
        outcome,
    }
}

fn no_op_plan() -> RustActiveNavigationDispatchPlan {
    boundary_outcome(RustActiveNavigationDispatchOutcome::NoOp)
}

#[allow(clippy::too_many_arguments)]
fn active_navigation_snapshot(
    available: bool,
    known: bool,
    editable: bool,
    can_open_previous: bool,
    can_open_next: bool,
    at_known_first: bool,
    at_known_last: bool,
    current_number: i32,
    count: i32,
) -> RustActiveNavigationSnapshot {
    RustActiveNavigationSnapshot {
        available,
        known,
        editable,
        can_open_previous,
        can_open_next,
        at_known_first,
        at_known_last,
        current_number,
        count,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn boundary_state(
        can_open_previous: bool,
        can_open_next: bool,
        at_known_first: bool,
        at_known_last: bool,
        current_number: i32,
        count: i32,
    ) -> RustDirectMediaActiveNavigationInput {
        RustDirectMediaActiveNavigationInput {
            boundary_state: ffi::RustDirectMediaNavigationBoundaryState {
                can_open_previous,
                can_open_next,
                at_known_first,
                at_known_last,
                current_number,
                count,
            },
            known: true,
        }
    }

    fn image_document_snapshot(
        can_open_previous: bool,
        can_open_next: bool,
        at_known_first: bool,
        at_known_last: bool,
        current_number: i32,
        count: i32,
    ) -> RustImageDocumentPageActiveNavigationSnapshot {
        RustImageDocumentPageActiveNavigationSnapshot {
            known: true,
            can_open_previous,
            can_open_next,
            at_known_first,
            at_known_last,
            current_number,
            count,
        }
    }

    fn dispatch_request(
        kind: RustActiveNavigationDispatchRequestKind,
        number: i32,
    ) -> RustActiveNavigationDispatchRequest {
        RustActiveNavigationDispatchRequest { kind, number }
    }

    #[test]
    fn unavailable_source_projects_unavailable() {
        assert_eq!(
            rust_project_active_navigation(
                RustActiveNavigationSourceKind::NoSource,
                RustDirectMediaActiveNavigationInput {
                    boundary_state: ffi::RustDirectMediaNavigationBoundaryState {
                        can_open_previous: false,
                        can_open_next: false,
                        at_known_first: false,
                        at_known_last: false,
                        current_number: 0,
                        count: 0,
                    },
                    known: false,
                },
                RustImageDocumentPageActiveNavigationSnapshot {
                    known: false,
                    can_open_previous: false,
                    can_open_next: false,
                    at_known_first: false,
                    at_known_last: false,
                    current_number: 0,
                    count: 0,
                },
                false,
            ),
            unavailable_active_navigation()
        );
    }

    #[test]
    fn direct_media_projects_known_boundary_state() {
        assert_eq!(
            rust_project_active_navigation(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                boundary_state(true, true, false, false, 2, 4),
                RustImageDocumentPageActiveNavigationSnapshot {
                    known: false,
                    can_open_previous: false,
                    can_open_next: false,
                    at_known_first: false,
                    at_known_last: false,
                    current_number: 0,
                    count: 0,
                },
                false,
            ),
            active_navigation_snapshot(true, true, true, true, true, false, false, 2, 4)
        );
    }

    #[test]
    fn invalid_numbers_normalize_to_unknown() {
        assert_eq!(
            rust_project_active_navigation(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                boundary_state(true, true, false, false, 0, 4),
                RustImageDocumentPageActiveNavigationSnapshot {
                    known: false,
                    can_open_previous: false,
                    can_open_next: false,
                    at_known_first: false,
                    at_known_last: false,
                    current_number: 0,
                    count: 0,
                },
                false,
            ),
            unknown_active_navigation()
        );
        assert_eq!(
            rust_project_active_navigation(
                RustActiveNavigationSourceKind::ImageDocumentPages,
                boundary_state(false, false, false, false, 0, 0),
                image_document_snapshot(true, false, false, true, 6, 5),
                false,
            ),
            unknown_active_navigation()
        );
    }

    #[test]
    fn deletion_masking_keeps_readout_but_disables_dispatch() {
        assert_eq!(
            rust_project_active_navigation(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                boundary_state(true, true, false, false, 2, 4),
                RustImageDocumentPageActiveNavigationSnapshot {
                    known: false,
                    can_open_previous: false,
                    can_open_next: false,
                    at_known_first: false,
                    at_known_last: false,
                    current_number: 0,
                    count: 0,
                },
                true,
            ),
            active_navigation_snapshot(true, true, false, false, false, false, false, 2, 4)
        );
    }

    #[test]
    fn source_kind_maps_to_boundary_scope() {
        assert_eq!(
            rust_active_navigation_boundary_scope_for_source(
                RustActiveNavigationSourceKind::NoSource,
            ),
            RustActiveNavigationBoundaryScope::NoBoundary
        );
        assert_eq!(
            rust_active_navigation_boundary_scope_for_source(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
            ),
            RustActiveNavigationBoundaryScope::DirectMedia
        );
        assert_eq!(
            rust_active_navigation_boundary_scope_for_source(
                RustActiveNavigationSourceKind::ImageDocumentPages,
            ),
            RustActiveNavigationBoundaryScope::ImageDocumentPage
        );
    }

    #[test]
    fn direct_media_dispatch_uses_direct_media_operations() {
        let snapshot = active_navigation_snapshot(true, true, true, true, true, false, false, 2, 4);

        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                snapshot,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Previous, 0),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind: RustActiveNavigationDispatchOperationKind::OpenPreviousDirectMedia,
                operation_number: 0,
                outcome: RustActiveNavigationDispatchOutcome::Dispatch,
            }
        );
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                snapshot,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Number, 3),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind: RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber,
                operation_number: 3,
                outcome: RustActiveNavigationDispatchOutcome::Dispatch,
            }
        );
    }

    #[test]
    fn numbered_dispatch_clamps_to_known_range() {
        let direct_media =
            active_navigation_snapshot(true, true, true, true, true, false, false, 2, 4);
        let image_document =
            active_navigation_snapshot(true, true, true, true, true, false, false, 2, 5);

        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                direct_media,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Number, 0),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind: RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber,
                operation_number: 1,
                outcome: RustActiveNavigationDispatchOutcome::Dispatch,
            }
        );
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::ImageDocumentPages,
                image_document,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Number, 8),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind:
                    RustActiveNavigationDispatchOperationKind::OpenImageDocumentPageAtNumber,
                operation_number: 5,
                outcome: RustActiveNavigationDispatchOutcome::Dispatch,
            }
        );
    }

    #[test]
    fn image_document_dispatch_uses_page_operations_and_boundaries() {
        let middle = active_navigation_snapshot(true, true, true, true, true, false, false, 2, 5);
        let first = active_navigation_snapshot(true, true, true, false, true, true, false, 1, 5);

        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::ImageDocumentPages,
                middle,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Last, 0),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind:
                    RustActiveNavigationDispatchOperationKind::OpenImageDocumentPageAtNumber,
                operation_number: 5,
                outcome: RustActiveNavigationDispatchOutcome::Dispatch,
            }
        );
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::ImageDocumentPages,
                first,
                dispatch_request(RustActiveNavigationDispatchRequestKind::Previous, 0),
            ),
            RustActiveNavigationDispatchPlan {
                operation_kind: RustActiveNavigationDispatchOperationKind::NoOperation,
                operation_number: 0,
                outcome: RustActiveNavigationDispatchOutcome::FirstBoundary,
            }
        );
    }

    #[test]
    fn dispatch_rejects_unknown_masked_and_unavailable_navigation() {
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                unknown_active_navigation(),
                dispatch_request(RustActiveNavigationDispatchRequestKind::Number, 1),
            ),
            no_op_plan()
        );
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::OrdinaryDirectMedia,
                active_navigation_snapshot(true, true, false, false, false, false, false, 2, 4),
                dispatch_request(RustActiveNavigationDispatchRequestKind::Previous, 0),
            ),
            no_op_plan()
        );
        assert_eq!(
            rust_active_navigation_dispatch_plan(
                RustActiveNavigationSourceKind::NoSource,
                active_navigation_snapshot(false, false, false, true, false, false, false, 0, 0),
                dispatch_request(RustActiveNavigationDispatchRequestKind::Previous, 0),
            ),
            no_op_plan()
        );
    }
}
