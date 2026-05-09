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
    enum RustImageDocumentNotificationChange {
        Status = 0,
        Loading = 1,
        ImageSize = 2,
        DisplaySize = 3,
        ZoomPercent = 4,
        ZoomMode = 5,
        MaximumManualZoomPercent = 6,
        TwoPageMode = 7,
        RightToLeftReading = 8,
        Repaint = 9,
        ViewportSize = 10,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentStateSnapshot {
        status: RustImageDocumentStatus,
        loading: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentZoomChangeSet {
        image_size_changed: bool,
        viewport_size_changed: bool,
        zoom_mode_changed: bool,
        zoom_percent_changed: bool,
        display_size_changed: bool,
        maximum_manual_zoom_percent_changed: bool,
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

        #[cxx_name = "rustImageDocumentSpreadTransitionNotifications"]
        fn rust_image_document_spread_transition_notifications()
        -> Vec<RustImageDocumentNotificationChange>;

        #[cxx_name = "rustImageDocumentTwoPageModeNotifications"]
        fn rust_image_document_two_page_mode_notifications()
        -> Vec<RustImageDocumentNotificationChange>;

        #[cxx_name = "rustImageDocumentSpreadZoomNotifications"]
        fn rust_image_document_spread_zoom_notifications()
        -> Vec<RustImageDocumentNotificationChange>;

        #[cxx_name = "rustImageDocumentRightToLeftReadingNotifications"]
        fn rust_image_document_right_to_left_reading_notifications(
            secondary_page_visible: bool,
        ) -> Vec<RustImageDocumentNotificationChange>;

        #[cxx_name = "rustImageDocumentPresentationZoomNotifications"]
        fn rust_image_document_presentation_zoom_notifications(
            changes: RustImageDocumentZoomChangeSet,
        ) -> Vec<RustImageDocumentNotificationChange>;
    }
}

use ffi::{
    RustImageDocumentNotificationChange, RustImageDocumentStateChange,
    RustImageDocumentStateSnapshot, RustImageDocumentStatus, RustImageDocumentZoomChangeSet,
};

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

fn rust_image_document_spread_transition_notifications() -> Vec<RustImageDocumentNotificationChange>
{
    vec![
        RustImageDocumentNotificationChange::Status,
        RustImageDocumentNotificationChange::Loading,
        RustImageDocumentNotificationChange::Repaint,
    ]
}

fn rust_image_document_two_page_mode_notifications() -> Vec<RustImageDocumentNotificationChange> {
    vec![
        RustImageDocumentNotificationChange::TwoPageMode,
        RustImageDocumentNotificationChange::ImageSize,
        RustImageDocumentNotificationChange::DisplaySize,
        RustImageDocumentNotificationChange::ZoomPercent,
        RustImageDocumentNotificationChange::ZoomMode,
        RustImageDocumentNotificationChange::MaximumManualZoomPercent,
        RustImageDocumentNotificationChange::Repaint,
    ]
}

fn rust_image_document_spread_zoom_notifications() -> Vec<RustImageDocumentNotificationChange> {
    vec![
        RustImageDocumentNotificationChange::ZoomMode,
        RustImageDocumentNotificationChange::ZoomPercent,
        RustImageDocumentNotificationChange::DisplaySize,
        RustImageDocumentNotificationChange::MaximumManualZoomPercent,
        RustImageDocumentNotificationChange::Repaint,
        RustImageDocumentNotificationChange::TwoPageMode,
    ]
}

fn rust_image_document_right_to_left_reading_notifications(
    secondary_page_visible: bool,
) -> Vec<RustImageDocumentNotificationChange> {
    let mut changes = vec![
        RustImageDocumentNotificationChange::RightToLeftReading,
        RustImageDocumentNotificationChange::Repaint,
    ];
    if secondary_page_visible {
        changes.push(RustImageDocumentNotificationChange::TwoPageMode);
    }
    changes
}

fn rust_image_document_presentation_zoom_notifications(
    changes: RustImageDocumentZoomChangeSet,
) -> Vec<RustImageDocumentNotificationChange> {
    let mut notifications = Vec::new();
    if changes.image_size_changed {
        notifications.push(RustImageDocumentNotificationChange::ImageSize);
    }
    if changes.viewport_size_changed {
        notifications.push(RustImageDocumentNotificationChange::ViewportSize);
    }
    if changes.zoom_mode_changed {
        notifications.push(RustImageDocumentNotificationChange::ZoomMode);
    }
    if changes.zoom_percent_changed {
        notifications.push(RustImageDocumentNotificationChange::ZoomPercent);
    }
    if changes.display_size_changed {
        notifications.push(RustImageDocumentNotificationChange::DisplaySize);
        notifications.push(RustImageDocumentNotificationChange::Repaint);
    }
    if changes.maximum_manual_zoom_percent_changed {
        notifications.push(RustImageDocumentNotificationChange::MaximumManualZoomPercent);
    }
    notifications
}

#[cfg(test)]
mod tests {
    use super::*;

    fn zoom_change_set() -> RustImageDocumentZoomChangeSet {
        RustImageDocumentZoomChangeSet {
            image_size_changed: false,
            viewport_size_changed: false,
            zoom_mode_changed: false,
            zoom_percent_changed: false,
            display_size_changed: false,
            maximum_manual_zoom_percent_changed: false,
        }
    }

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

    #[test]
    fn document_notification_plans_preserve_existing_emission_order() {
        assert_eq!(
            rust_image_document_spread_transition_notifications(),
            vec![
                RustImageDocumentNotificationChange::Status,
                RustImageDocumentNotificationChange::Loading,
                RustImageDocumentNotificationChange::Repaint,
            ]
        );
        assert_eq!(
            rust_image_document_two_page_mode_notifications(),
            vec![
                RustImageDocumentNotificationChange::TwoPageMode,
                RustImageDocumentNotificationChange::ImageSize,
                RustImageDocumentNotificationChange::DisplaySize,
                RustImageDocumentNotificationChange::ZoomPercent,
                RustImageDocumentNotificationChange::ZoomMode,
                RustImageDocumentNotificationChange::MaximumManualZoomPercent,
                RustImageDocumentNotificationChange::Repaint,
            ]
        );
        assert_eq!(
            rust_image_document_spread_zoom_notifications(),
            vec![
                RustImageDocumentNotificationChange::ZoomMode,
                RustImageDocumentNotificationChange::ZoomPercent,
                RustImageDocumentNotificationChange::DisplaySize,
                RustImageDocumentNotificationChange::MaximumManualZoomPercent,
                RustImageDocumentNotificationChange::Repaint,
                RustImageDocumentNotificationChange::TwoPageMode,
            ]
        );
    }

    #[test]
    fn right_to_left_reading_notifications_include_two_page_mode_only_when_visible() {
        assert_eq!(
            rust_image_document_right_to_left_reading_notifications(false),
            vec![
                RustImageDocumentNotificationChange::RightToLeftReading,
                RustImageDocumentNotificationChange::Repaint,
            ]
        );
        assert_eq!(
            rust_image_document_right_to_left_reading_notifications(true),
            vec![
                RustImageDocumentNotificationChange::RightToLeftReading,
                RustImageDocumentNotificationChange::Repaint,
                RustImageDocumentNotificationChange::TwoPageMode,
            ]
        );
    }

    #[test]
    fn presentation_zoom_notifications_preserve_existing_emission_order() {
        let notifications =
            rust_image_document_presentation_zoom_notifications(RustImageDocumentZoomChangeSet {
                image_size_changed: true,
                viewport_size_changed: true,
                zoom_mode_changed: true,
                zoom_percent_changed: true,
                display_size_changed: true,
                maximum_manual_zoom_percent_changed: true,
            });

        assert_eq!(
            notifications,
            vec![
                RustImageDocumentNotificationChange::ImageSize,
                RustImageDocumentNotificationChange::ViewportSize,
                RustImageDocumentNotificationChange::ZoomMode,
                RustImageDocumentNotificationChange::ZoomPercent,
                RustImageDocumentNotificationChange::DisplaySize,
                RustImageDocumentNotificationChange::Repaint,
                RustImageDocumentNotificationChange::MaximumManualZoomPercent,
            ]
        );
    }

    #[test]
    fn presentation_zoom_notifications_do_not_emit_without_notification_changes() {
        assert!(rust_image_document_presentation_zoom_notifications(zoom_change_set()).is_empty());
    }
}
