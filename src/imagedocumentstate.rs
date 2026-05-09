// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentStatus {
        Null = 0,
        Loading = 1,
        Ready = 2,
        Error = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentStateSnapshot {
        status: RustImageDocumentStatus,
        loading: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentStateChange {
        changed: bool,
        snapshot: RustImageDocumentStateSnapshot,
    }

    extern "Rust" {
        #[cxx_name = "rustImageDocumentStateSnapshot"]
        fn rust_image_document_state_snapshot(
            status: RustImageDocumentStatus,
            loading: bool,
        ) -> RustImageDocumentStateSnapshot;

        #[cxx_name = "rustImageDocumentSetStatus"]
        fn rust_image_document_set_status(
            snapshot: RustImageDocumentStateSnapshot,
            status: RustImageDocumentStatus,
        ) -> RustImageDocumentStateChange;

        #[cxx_name = "rustImageDocumentSetLoading"]
        fn rust_image_document_set_loading(
            snapshot: RustImageDocumentStateSnapshot,
            loading: bool,
        ) -> RustImageDocumentStateChange;
    }
}

use ffi::{RustImageDocumentStateChange, RustImageDocumentStateSnapshot, RustImageDocumentStatus};

fn rust_image_document_state_snapshot(
    status: RustImageDocumentStatus,
    loading: bool,
) -> RustImageDocumentStateSnapshot {
    RustImageDocumentStateSnapshot { status, loading }
}

fn rust_image_document_set_status(
    snapshot: RustImageDocumentStateSnapshot,
    status: RustImageDocumentStatus,
) -> RustImageDocumentStateChange {
    let mut next = snapshot;
    next.status = status;
    state_change(snapshot, next)
}

fn rust_image_document_set_loading(
    snapshot: RustImageDocumentStateSnapshot,
    loading: bool,
) -> RustImageDocumentStateChange {
    let mut next = snapshot;
    next.loading = loading;
    state_change(snapshot, next)
}

fn state_change(
    current: RustImageDocumentStateSnapshot,
    next: RustImageDocumentStateSnapshot,
) -> RustImageDocumentStateChange {
    RustImageDocumentStateChange {
        changed: current != next,
        snapshot: next,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn snapshot_preserves_status_and_loading() {
        let snapshot = rust_image_document_state_snapshot(RustImageDocumentStatus::Ready, true);

        assert_eq!(snapshot.status, RustImageDocumentStatus::Ready);
        assert!(snapshot.loading);
    }

    #[test]
    fn status_reducer_reports_changed_and_unchanged_states() {
        let snapshot = rust_image_document_state_snapshot(RustImageDocumentStatus::Null, false);

        let changed = rust_image_document_set_status(snapshot, RustImageDocumentStatus::Loading);
        assert!(changed.changed);
        assert_eq!(changed.snapshot.status, RustImageDocumentStatus::Loading);
        assert!(!changed.snapshot.loading);

        let unchanged = rust_image_document_set_status(snapshot, RustImageDocumentStatus::Null);
        assert!(!unchanged.changed);
        assert_eq!(unchanged.snapshot, snapshot);
    }

    #[test]
    fn loading_reducer_reports_changed_and_unchanged_states() {
        let snapshot = rust_image_document_state_snapshot(RustImageDocumentStatus::Ready, false);

        let changed = rust_image_document_set_loading(snapshot, true);
        assert!(changed.changed);
        assert_eq!(changed.snapshot.status, RustImageDocumentStatus::Ready);
        assert!(changed.snapshot.loading);

        let unchanged = rust_image_document_set_loading(snapshot, false);
        assert!(!unchanged.changed);
        assert_eq!(unchanged.snapshot, snapshot);
    }
}
