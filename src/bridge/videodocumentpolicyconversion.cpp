// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/videodocumentpolicyconversion.h"

namespace {
KiriView::RustVideoMediaStatus rustVideoMediaStatus(KiriView::VideoMediaStatus status)
{
    switch (status) {
    case KiriView::VideoMediaStatus::Null:
        return KiriView::RustVideoMediaStatus::Null;
    case KiriView::VideoMediaStatus::Loading:
        return KiriView::RustVideoMediaStatus::Loading;
    case KiriView::VideoMediaStatus::Loaded:
        return KiriView::RustVideoMediaStatus::Loaded;
    case KiriView::VideoMediaStatus::Stalled:
        return KiriView::RustVideoMediaStatus::Stalled;
    case KiriView::VideoMediaStatus::Buffering:
        return KiriView::RustVideoMediaStatus::Buffering;
    case KiriView::VideoMediaStatus::Buffered:
        return KiriView::RustVideoMediaStatus::Buffered;
    case KiriView::VideoMediaStatus::EndOfMedia:
        return KiriView::RustVideoMediaStatus::EndOfMedia;
    case KiriView::VideoMediaStatus::Invalid:
        return KiriView::RustVideoMediaStatus::Invalid;
    }

    return KiriView::RustVideoMediaStatus::Null;
}

KiriView::VideoDocumentStatus videoDocumentStatus(KiriView::RustVideoDocumentStatus status)
{
    switch (status) {
    case KiriView::RustVideoDocumentStatus::Null:
        return KiriView::VideoDocumentStatus::Null;
    case KiriView::RustVideoDocumentStatus::Loading:
        return KiriView::VideoDocumentStatus::Loading;
    case KiriView::RustVideoDocumentStatus::Ready:
        return KiriView::VideoDocumentStatus::Ready;
    case KiriView::RustVideoDocumentStatus::Error:
        return KiriView::VideoDocumentStatus::Error;
    }

    return KiriView::VideoDocumentStatus::Null;
}

KiriView::VideoPlaybackBackendOperation videoPlaybackBackendOperation(
    const KiriView::RustVideoPlaybackBackendOperation &operation)
{
    switch (operation.kind) {
    case KiriView::RustVideoPlaybackBackendOperationKind::EnsureBackend:
        return KiriView::EnsureVideoPlaybackBackendOperation {};
    case KiriView::RustVideoPlaybackBackendOperationKind::Play:
        return KiriView::PlayVideoPlaybackOperation {};
    case KiriView::RustVideoPlaybackBackendOperationKind::Pause:
        return KiriView::PauseVideoPlaybackOperation {};
    case KiriView::RustVideoPlaybackBackendOperationKind::Stop:
        return KiriView::StopVideoPlaybackOperation {};
    case KiriView::RustVideoPlaybackBackendOperationKind::SetPosition:
        return KiriView::SetVideoPlaybackPositionOperation { operation.position };
    }

    return KiriView::EnsureVideoPlaybackBackendOperation {};
}
}

namespace KiriView::Bridge {
RustVideoDocumentStatusSnapshot rustVideoDocumentStatusSnapshot(
    const VideoDocumentStatusSnapshot &snapshot)
{
    return RustVideoDocumentStatusSnapshot {
        snapshot.sourceUrlEmpty,
        snapshot.sourceLoadActive,
        snapshot.mediaBackendAvailable,
        rustVideoMediaStatus(snapshot.mediaStatus),
    };
}

VideoDocumentStatusPlan videoDocumentStatusPlanFromRust(const RustVideoDocumentStatusPlan &plan)
{
    return VideoDocumentStatusPlan {
        videoDocumentStatus(plan.status),
        plan.media_ended,
        plan.clear_playing,
    };
}

RustVideoPlaybackControlSnapshot rustVideoPlaybackControlSnapshot(
    const VideoPlaybackControlSnapshot &snapshot)
{
    return RustVideoPlaybackControlSnapshot {
        snapshot.sourceUrlEmpty,
        snapshot.mediaBackendAvailable,
        snapshot.playing,
        snapshot.mediaEnded,
        snapshot.seekable,
        snapshot.position,
        snapshot.duration,
    };
}

VideoPlaybackControlPlan videoPlaybackControlPlanFromRust(
    const RustVideoPlaybackControlPlan &rustPlan)
{
    VideoPlaybackControlPlan plan;
    const RustVideoPlaybackStateDelta &delta = rustPlan.state_delta;
    if (delta.media_ended_changed) {
        plan.stateDelta.mediaEnded = delta.media_ended;
    }
    if (delta.playing_changed) {
        plan.stateDelta.playing = delta.playing;
    }
    if (delta.position_changed) {
        plan.stateDelta.position = delta.position;
    }

    plan.backendOperations.reserve(rustPlan.backend_operations.size());
    for (const RustVideoPlaybackBackendOperation &operation : rustPlan.backend_operations) {
        plan.backendOperations.push_back(videoPlaybackBackendOperation(operation));
    }
    return plan;
}
}
