// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/videodocumentpolicyconversion.h"

namespace {
kiriview::RustVideoMediaStatus rustVideoMediaStatus(kiriview::VideoMediaStatus status)
{
    switch (status) {
    case kiriview::VideoMediaStatus::Null:
        return kiriview::RustVideoMediaStatus::Null;
    case kiriview::VideoMediaStatus::Loading:
        return kiriview::RustVideoMediaStatus::Loading;
    case kiriview::VideoMediaStatus::Loaded:
        return kiriview::RustVideoMediaStatus::Loaded;
    case kiriview::VideoMediaStatus::Stalled:
        return kiriview::RustVideoMediaStatus::Stalled;
    case kiriview::VideoMediaStatus::Buffering:
        return kiriview::RustVideoMediaStatus::Buffering;
    case kiriview::VideoMediaStatus::Buffered:
        return kiriview::RustVideoMediaStatus::Buffered;
    case kiriview::VideoMediaStatus::EndOfMedia:
        return kiriview::RustVideoMediaStatus::EndOfMedia;
    case kiriview::VideoMediaStatus::Invalid:
        return kiriview::RustVideoMediaStatus::Invalid;
    }

    return kiriview::RustVideoMediaStatus::Null;
}

kiriview::VideoDocumentStatus videoDocumentStatus(kiriview::RustVideoDocumentStatus status)
{
    switch (status) {
    case kiriview::RustVideoDocumentStatus::Null:
        return kiriview::VideoDocumentStatus::Null;
    case kiriview::RustVideoDocumentStatus::Loading:
        return kiriview::VideoDocumentStatus::Loading;
    case kiriview::RustVideoDocumentStatus::Ready:
        return kiriview::VideoDocumentStatus::Ready;
    case kiriview::RustVideoDocumentStatus::Error:
        return kiriview::VideoDocumentStatus::Error;
    }

    return kiriview::VideoDocumentStatus::Null;
}

kiriview::VideoPlaybackBackendOperation videoPlaybackBackendOperation(
    const kiriview::RustVideoPlaybackBackendOperation &operation)
{
    switch (operation.kind) {
    case kiriview::RustVideoPlaybackBackendOperationKind::EnsureBackend:
        return kiriview::EnsureVideoPlaybackBackendOperation {};
    case kiriview::RustVideoPlaybackBackendOperationKind::Play:
        return kiriview::PlayVideoPlaybackOperation {};
    case kiriview::RustVideoPlaybackBackendOperationKind::Pause:
        return kiriview::PauseVideoPlaybackOperation {};
    case kiriview::RustVideoPlaybackBackendOperationKind::Stop:
        return kiriview::StopVideoPlaybackOperation {};
    case kiriview::RustVideoPlaybackBackendOperationKind::SetPosition:
        return kiriview::SetVideoPlaybackPositionOperation { operation.position };
    }

    return kiriview::EnsureVideoPlaybackBackendOperation {};
}
}

namespace kiriview::Bridge {
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
