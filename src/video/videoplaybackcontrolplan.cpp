// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackcontrolplan.h"

#include <algorithm>

namespace {
using KiriView::EnsureVideoPlaybackBackendOperation;
using KiriView::PauseVideoPlaybackOperation;
using KiriView::PlayVideoPlaybackOperation;
using KiriView::SetVideoPlaybackPositionOperation;
using KiriView::StopVideoPlaybackOperation;
using KiriView::VideoPlaybackControlPlan;
using KiriView::VideoPlaybackControlSnapshot;

SetVideoPlaybackPositionOperation setPositionOperation(qint64 position)
{
    return SetVideoPlaybackPositionOperation { position };
}

void appendEnsureBackend(VideoPlaybackControlPlan &plan)
{
    plan.backendOperations.push_back(EnsureVideoPlaybackBackendOperation {});
}

qint64 clampedAbsolutePosition(qint64 position, qint64 duration)
{
    if (duration > 0) {
        return std::clamp(position, qint64(0), duration);
    }

    return std::max<qint64>(0, position);
}
}

namespace KiriView {
bool VideoPlaybackControlPlan::isEmpty() const
{
    return !stateDelta.mediaEnded.has_value() && !stateDelta.playing.has_value()
        && !stateDelta.position.has_value() && backendOperations.empty();
}

VideoPlaybackControlPlan videoPlaybackPlayPlan(VideoPlaybackControlSnapshot snapshot)
{
    VideoPlaybackControlPlan plan;
    if (snapshot.sourceUrlEmpty) {
        return plan;
    }

    appendEnsureBackend(plan);
    if (snapshot.mediaEnded && snapshot.seekable) {
        plan.backendOperations.push_back(setPositionOperation(0));
        plan.stateDelta.position = 0;
    }
    plan.stateDelta.mediaEnded = false;
    plan.backendOperations.push_back(PlayVideoPlaybackOperation {});
    return plan;
}

VideoPlaybackControlPlan videoPlaybackPausePlan(VideoPlaybackControlSnapshot snapshot)
{
    VideoPlaybackControlPlan plan;
    if (!snapshot.mediaBackendAvailable) {
        return plan;
    }

    plan.backendOperations.push_back(PauseVideoPlaybackOperation {});
    return plan;
}

VideoPlaybackControlPlan videoPlaybackStopPlan(VideoPlaybackControlSnapshot snapshot)
{
    VideoPlaybackControlPlan plan;
    plan.stateDelta.mediaEnded = false;
    if (snapshot.mediaBackendAvailable) {
        plan.backendOperations.push_back(StopVideoPlaybackOperation {});
    }
    plan.stateDelta.playing = false;
    if (snapshot.seekable) {
        if (snapshot.mediaBackendAvailable) {
            plan.backendOperations.push_back(setPositionOperation(0));
        }
        plan.stateDelta.position = 0;
    }
    return plan;
}

VideoPlaybackControlPlan videoPlaybackTogglePlan(VideoPlaybackControlSnapshot snapshot)
{
    if (snapshot.playing) {
        return videoPlaybackPausePlan(snapshot);
    }

    return videoPlaybackPlayPlan(snapshot);
}

VideoPlaybackControlPlan videoPlaybackSetPositionPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 position)
{
    VideoPlaybackControlPlan plan;
    if (!snapshot.seekable) {
        return plan;
    }

    const qint64 clampedPosition = clampedAbsolutePosition(position, snapshot.duration);
    plan.stateDelta.mediaEnded = false;
    appendEnsureBackend(plan);
    plan.backendOperations.push_back(setPositionOperation(clampedPosition));
    plan.stateDelta.position = clampedPosition;
    return plan;
}

VideoPlaybackControlPlan videoPlaybackSeekByPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 deltaMilliseconds)
{
    const qint64 nextPosition = videoPlaybackClampedSeekPosition(
        snapshot.position, deltaMilliseconds, snapshot.duration, snapshot.seekable);
    if (!snapshot.seekable || nextPosition == snapshot.position) {
        return {};
    }

    VideoPlaybackControlPlan plan;
    plan.stateDelta.mediaEnded = false;
    appendEnsureBackend(plan);
    plan.backendOperations.push_back(setPositionOperation(nextPosition));
    plan.stateDelta.position = nextPosition;
    return plan;
}

qint64 videoPlaybackClampedSeekPosition(
    qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable)
{
    if (!seekable) {
        return currentPosition;
    }

    return clampedAbsolutePosition(currentPosition + deltaMilliseconds, duration);
}
}
