// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustVideoDocumentStatus {
        Null = 0,
        Loading = 1,
        Ready = 2,
        Error = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustVideoMediaStatus {
        Null = 0,
        Loading = 1,
        Loaded = 2,
        Stalled = 3,
        Buffering = 4,
        Buffered = 5,
        EndOfMedia = 6,
        Invalid = 7,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustVideoPlaybackBackendOperationKind {
        EnsureBackend = 0,
        Play = 1,
        Pause = 2,
        Stop = 3,
        SetPosition = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoDocumentStatusSnapshot {
        source_url_empty: bool,
        source_load_active: bool,
        media_backend_available: bool,
        media_status: RustVideoMediaStatus,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoDocumentStatusPlan {
        status: RustVideoDocumentStatus,
        media_ended: bool,
        clear_playing: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoPlaybackControlSnapshot {
        source_url_empty: bool,
        media_backend_available: bool,
        playing: bool,
        media_ended: bool,
        seekable: bool,
        position: i64,
        duration: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoPlaybackStateDelta {
        media_ended_changed: bool,
        media_ended: bool,
        playing_changed: bool,
        playing: bool,
        position_changed: bool,
        position: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustVideoPlaybackBackendOperation {
        kind: RustVideoPlaybackBackendOperationKind,
        position: i64,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustVideoPlaybackControlPlan {
        state_delta: RustVideoPlaybackStateDelta,
        backend_operations: Vec<RustVideoPlaybackBackendOperation>,
    }

    extern "Rust" {
        #[cxx_name = "rustVideoDocumentStatusPlan"]
        fn rust_video_document_status_plan(
            snapshot: RustVideoDocumentStatusSnapshot,
        ) -> RustVideoDocumentStatusPlan;

        #[cxx_name = "rustVideoPlaybackPlayPlan"]
        fn rust_video_playback_play_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackPausePlan"]
        fn rust_video_playback_pause_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackStopPlan"]
        fn rust_video_playback_stop_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackTogglePlan"]
        fn rust_video_playback_toggle_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackSetPositionPlan"]
        fn rust_video_playback_set_position_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
            position: i64,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackSeekByPlan"]
        fn rust_video_playback_seek_by_plan(
            snapshot: RustVideoPlaybackControlSnapshot,
            delta_milliseconds: i64,
        ) -> RustVideoPlaybackControlPlan;

        #[cxx_name = "rustVideoPlaybackClampedSeekPosition"]
        fn rust_video_playback_clamped_seek_position(
            current_position: i64,
            delta_milliseconds: i64,
            duration: i64,
            seekable: bool,
        ) -> i64;
    }
}

use ffi::{
    RustVideoDocumentStatus, RustVideoDocumentStatusPlan, RustVideoDocumentStatusSnapshot,
    RustVideoMediaStatus, RustVideoPlaybackBackendOperation, RustVideoPlaybackBackendOperationKind,
    RustVideoPlaybackControlPlan, RustVideoPlaybackControlSnapshot, RustVideoPlaybackStateDelta,
};

fn rust_video_document_status_plan(
    snapshot: RustVideoDocumentStatusSnapshot,
) -> RustVideoDocumentStatusPlan {
    if snapshot.source_url_empty {
        return RustVideoDocumentStatusPlan {
            status: RustVideoDocumentStatus::Null,
            media_ended: false,
            clear_playing: false,
        };
    }

    if snapshot.source_load_active || !snapshot.media_backend_available {
        return RustVideoDocumentStatusPlan {
            status: RustVideoDocumentStatus::Loading,
            media_ended: false,
            clear_playing: false,
        };
    }

    let media_ended = snapshot.media_status == RustVideoMediaStatus::EndOfMedia;
    RustVideoDocumentStatusPlan {
        status: document_status_for_media_status(snapshot.media_status),
        media_ended,
        clear_playing: media_ended,
    }
}

fn document_status_for_media_status(status: RustVideoMediaStatus) -> RustVideoDocumentStatus {
    match status {
        RustVideoMediaStatus::Null
        | RustVideoMediaStatus::Loading
        | RustVideoMediaStatus::Stalled => RustVideoDocumentStatus::Loading,
        RustVideoMediaStatus::Loaded
        | RustVideoMediaStatus::Buffering
        | RustVideoMediaStatus::Buffered
        | RustVideoMediaStatus::EndOfMedia => RustVideoDocumentStatus::Ready,
        RustVideoMediaStatus::Invalid => RustVideoDocumentStatus::Error,
        _ => RustVideoDocumentStatus::Loading,
    }
}

fn rust_video_playback_play_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
) -> RustVideoPlaybackControlPlan {
    let mut plan = empty_playback_control_plan();
    if snapshot.source_url_empty {
        return plan;
    }

    push_backend_operation(
        &mut plan,
        RustVideoPlaybackBackendOperationKind::EnsureBackend,
    );
    if snapshot.media_ended && snapshot.seekable {
        push_set_position_operation(&mut plan, 0);
        set_position_delta(&mut plan, 0);
    }
    set_media_ended_delta(&mut plan, false);
    push_backend_operation(&mut plan, RustVideoPlaybackBackendOperationKind::Play);
    plan
}

fn rust_video_playback_pause_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
) -> RustVideoPlaybackControlPlan {
    let mut plan = empty_playback_control_plan();
    if !snapshot.media_backend_available {
        return plan;
    }

    push_backend_operation(&mut plan, RustVideoPlaybackBackendOperationKind::Pause);
    plan
}

fn rust_video_playback_stop_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
) -> RustVideoPlaybackControlPlan {
    let mut plan = empty_playback_control_plan();
    set_media_ended_delta(&mut plan, false);
    if snapshot.media_backend_available {
        push_backend_operation(&mut plan, RustVideoPlaybackBackendOperationKind::Stop);
    }
    set_playing_delta(&mut plan, false);
    if snapshot.seekable {
        if snapshot.media_backend_available {
            push_set_position_operation(&mut plan, 0);
        }
        set_position_delta(&mut plan, 0);
    }
    plan
}

fn rust_video_playback_toggle_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
) -> RustVideoPlaybackControlPlan {
    if snapshot.playing {
        rust_video_playback_pause_plan(snapshot)
    } else {
        rust_video_playback_play_plan(snapshot)
    }
}

fn rust_video_playback_set_position_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
    position: i64,
) -> RustVideoPlaybackControlPlan {
    let mut plan = empty_playback_control_plan();
    if !snapshot.seekable {
        return plan;
    }

    let clamped_position = clamped_absolute_position(position, snapshot.duration);
    set_media_ended_delta(&mut plan, false);
    push_backend_operation(
        &mut plan,
        RustVideoPlaybackBackendOperationKind::EnsureBackend,
    );
    push_set_position_operation(&mut plan, clamped_position);
    set_position_delta(&mut plan, clamped_position);
    plan
}

fn rust_video_playback_seek_by_plan(
    snapshot: RustVideoPlaybackControlSnapshot,
    delta_milliseconds: i64,
) -> RustVideoPlaybackControlPlan {
    let next_position = rust_video_playback_clamped_seek_position(
        snapshot.position,
        delta_milliseconds,
        snapshot.duration,
        snapshot.seekable,
    );
    if !snapshot.seekable || next_position == snapshot.position {
        return empty_playback_control_plan();
    }

    let mut plan = empty_playback_control_plan();
    set_media_ended_delta(&mut plan, false);
    push_backend_operation(
        &mut plan,
        RustVideoPlaybackBackendOperationKind::EnsureBackend,
    );
    push_set_position_operation(&mut plan, next_position);
    set_position_delta(&mut plan, next_position);
    plan
}

fn rust_video_playback_clamped_seek_position(
    current_position: i64,
    delta_milliseconds: i64,
    duration: i64,
    seekable: bool,
) -> i64 {
    if !seekable {
        return current_position;
    }

    clamped_absolute_position(current_position + delta_milliseconds, duration)
}

fn clamped_absolute_position(position: i64, duration: i64) -> i64 {
    if duration > 0 {
        position.clamp(0, duration)
    } else {
        position.max(0)
    }
}

fn empty_playback_control_plan() -> RustVideoPlaybackControlPlan {
    RustVideoPlaybackControlPlan {
        state_delta: RustVideoPlaybackStateDelta {
            media_ended_changed: false,
            media_ended: false,
            playing_changed: false,
            playing: false,
            position_changed: false,
            position: 0,
        },
        backend_operations: Vec::new(),
    }
}

fn push_backend_operation(
    plan: &mut RustVideoPlaybackControlPlan,
    kind: RustVideoPlaybackBackendOperationKind,
) {
    plan.backend_operations
        .push(RustVideoPlaybackBackendOperation { kind, position: 0 });
}

fn push_set_position_operation(plan: &mut RustVideoPlaybackControlPlan, position: i64) {
    plan.backend_operations
        .push(RustVideoPlaybackBackendOperation {
            kind: RustVideoPlaybackBackendOperationKind::SetPosition,
            position,
        });
}

fn set_media_ended_delta(plan: &mut RustVideoPlaybackControlPlan, media_ended: bool) {
    plan.state_delta.media_ended_changed = true;
    plan.state_delta.media_ended = media_ended;
}

fn set_playing_delta(plan: &mut RustVideoPlaybackControlPlan, playing: bool) {
    plan.state_delta.playing_changed = true;
    plan.state_delta.playing = playing;
}

fn set_position_delta(plan: &mut RustVideoPlaybackControlPlan, position: i64) {
    plan.state_delta.position_changed = true;
    plan.state_delta.position = position;
}

#[cfg(test)]
mod tests {
    use super::*;

    fn playable_snapshot() -> RustVideoPlaybackControlSnapshot {
        RustVideoPlaybackControlSnapshot {
            source_url_empty: false,
            media_backend_available: true,
            playing: false,
            media_ended: false,
            seekable: true,
            position: 5000,
            duration: 10000,
        }
    }

    fn operation_kinds(
        plan: &RustVideoPlaybackControlPlan,
    ) -> Vec<RustVideoPlaybackBackendOperationKind> {
        plan.backend_operations
            .iter()
            .map(|operation| operation.kind)
            .collect()
    }

    #[test]
    fn status_plan_preserves_source_and_resolver_precedence() {
        assert_eq!(
            rust_video_document_status_plan(RustVideoDocumentStatusSnapshot {
                source_url_empty: true,
                source_load_active: false,
                media_backend_available: true,
                media_status: RustVideoMediaStatus::Buffered,
            }),
            RustVideoDocumentStatusPlan {
                status: RustVideoDocumentStatus::Null,
                media_ended: false,
                clear_playing: false,
            }
        );
        assert_eq!(
            rust_video_document_status_plan(RustVideoDocumentStatusSnapshot {
                source_url_empty: false,
                source_load_active: true,
                media_backend_available: true,
                media_status: RustVideoMediaStatus::Invalid,
            })
            .status,
            RustVideoDocumentStatus::Loading
        );
        assert_eq!(
            rust_video_document_status_plan(RustVideoDocumentStatusSnapshot {
                source_url_empty: false,
                source_load_active: false,
                media_backend_available: false,
                media_status: RustVideoMediaStatus::Invalid,
            })
            .status,
            RustVideoDocumentStatus::Loading
        );
    }

    #[test]
    fn status_plan_maps_backend_statuses() {
        let cases = [
            (RustVideoMediaStatus::Null, RustVideoDocumentStatus::Loading),
            (
                RustVideoMediaStatus::Loading,
                RustVideoDocumentStatus::Loading,
            ),
            (RustVideoMediaStatus::Loaded, RustVideoDocumentStatus::Ready),
            (
                RustVideoMediaStatus::Stalled,
                RustVideoDocumentStatus::Loading,
            ),
            (
                RustVideoMediaStatus::Buffering,
                RustVideoDocumentStatus::Ready,
            ),
            (
                RustVideoMediaStatus::Buffered,
                RustVideoDocumentStatus::Ready,
            ),
            (
                RustVideoMediaStatus::Invalid,
                RustVideoDocumentStatus::Error,
            ),
        ];

        for (media_status, document_status) in cases {
            let plan = rust_video_document_status_plan(RustVideoDocumentStatusSnapshot {
                source_url_empty: false,
                source_load_active: false,
                media_backend_available: true,
                media_status,
            });
            assert_eq!(plan.status, document_status);
            assert!(!plan.media_ended);
            assert!(!plan.clear_playing);
        }
    }

    #[test]
    fn end_of_media_remains_ready_and_clears_playing() {
        let plan = rust_video_document_status_plan(RustVideoDocumentStatusSnapshot {
            source_url_empty: false,
            source_load_active: false,
            media_backend_available: true,
            media_status: RustVideoMediaStatus::EndOfMedia,
        });

        assert_eq!(plan.status, RustVideoDocumentStatus::Ready);
        assert!(plan.media_ended);
        assert!(plan.clear_playing);
    }

    #[test]
    fn play_restarts_seekable_end_of_media_before_playing() {
        let mut snapshot = playable_snapshot();
        snapshot.media_ended = true;
        snapshot.position = 10000;

        let plan = rust_video_playback_play_plan(snapshot);

        assert_eq!(
            operation_kinds(&plan),
            vec![
                RustVideoPlaybackBackendOperationKind::EnsureBackend,
                RustVideoPlaybackBackendOperationKind::SetPosition,
                RustVideoPlaybackBackendOperationKind::Play,
            ]
        );
        assert_eq!(plan.backend_operations[1].position, 0);
        assert!(plan.state_delta.media_ended_changed);
        assert!(!plan.state_delta.media_ended);
        assert!(plan.state_delta.position_changed);
        assert_eq!(plan.state_delta.position, 0);
        assert!(!plan.state_delta.playing_changed);
    }

    #[test]
    fn pause_requires_existing_backend() {
        let mut snapshot = playable_snapshot();
        snapshot.media_backend_available = false;
        assert!(
            rust_video_playback_pause_plan(snapshot)
                .backend_operations
                .is_empty()
        );

        snapshot.media_backend_available = true;
        let plan = rust_video_playback_pause_plan(snapshot);
        assert_eq!(
            operation_kinds(&plan),
            vec![RustVideoPlaybackBackendOperationKind::Pause]
        );
        assert!(!plan.state_delta.playing_changed);
    }

    #[test]
    fn stop_does_not_create_backend() {
        let mut snapshot = playable_snapshot();
        snapshot.media_backend_available = false;
        snapshot.playing = true;
        snapshot.position = 4000;

        let plan = rust_video_playback_stop_plan(snapshot);

        assert!(plan.backend_operations.is_empty());
        assert!(plan.state_delta.media_ended_changed);
        assert!(!plan.state_delta.media_ended);
        assert!(plan.state_delta.playing_changed);
        assert!(!plan.state_delta.playing);
        assert!(plan.state_delta.position_changed);
        assert_eq!(plan.state_delta.position, 0);
    }

    #[test]
    fn seek_plans_clamp_and_noop_when_unchanged_or_unseekable() {
        let mut snapshot = playable_snapshot();
        let plan = rust_video_playback_seek_by_plan(snapshot, 7000);

        assert_eq!(
            operation_kinds(&plan),
            vec![
                RustVideoPlaybackBackendOperationKind::EnsureBackend,
                RustVideoPlaybackBackendOperationKind::SetPosition,
            ]
        );
        assert_eq!(plan.backend_operations[1].position, 10000);
        assert_eq!(plan.state_delta.position, 10000);
        assert_eq!(
            rust_video_playback_clamped_seek_position(5000, 7000, 10000, true),
            10000
        );

        snapshot.position = 0;
        assert!(
            rust_video_playback_seek_by_plan(snapshot, -5000)
                .backend_operations
                .is_empty()
        );

        snapshot.seekable = false;
        snapshot.position = 5000;
        assert!(
            rust_video_playback_seek_by_plan(snapshot, 1000)
                .backend_operations
                .is_empty()
        );
        assert_eq!(
            rust_video_playback_clamped_seek_position(5000, 1000, 10000, false),
            5000
        );
    }
}
