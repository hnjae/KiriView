// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackcontrolplan.h"

#include "bridge/videodocumentpolicyconversion.h"
#include "kiriview/src/policy/videodocumentpolicy.cxx.h"

namespace {
using kiriview::VideoPlaybackControlPlan;
using kiriview::VideoPlaybackControlSnapshot;

kiriview::RustVideoPlaybackControlSnapshot rustSnapshot(VideoPlaybackControlSnapshot snapshot)
{
    return kiriview::Bridge::rustVideoPlaybackControlSnapshot(snapshot);
}
}

namespace kiriview {
bool VideoPlaybackControlPlan::isEmpty() const
{
    return !stateDelta.mediaEnded.has_value() && !stateDelta.playing.has_value()
        && !stateDelta.position.has_value() && backendOperations.empty();
}

VideoPlaybackControlPlan videoPlaybackPlayPlan(VideoPlaybackControlSnapshot snapshot)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackPlayPlan(rustSnapshot(snapshot)));
}

VideoPlaybackControlPlan videoPlaybackPausePlan(VideoPlaybackControlSnapshot snapshot)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackPausePlan(rustSnapshot(snapshot)));
}

VideoPlaybackControlPlan videoPlaybackStopPlan(VideoPlaybackControlSnapshot snapshot)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackStopPlan(rustSnapshot(snapshot)));
}

VideoPlaybackControlPlan videoPlaybackTogglePlan(VideoPlaybackControlSnapshot snapshot)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackTogglePlan(rustSnapshot(snapshot)));
}

VideoPlaybackControlPlan videoPlaybackSetPositionPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 position)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackSetPositionPlan(rustSnapshot(snapshot), position));
}

VideoPlaybackControlPlan videoPlaybackSeekByPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 deltaMilliseconds)
{
    return Bridge::videoPlaybackControlPlanFromRust(
        rustVideoPlaybackSeekByPlan(rustSnapshot(snapshot), deltaMilliseconds));
}

qint64 videoPlaybackClampedSeekPosition(
    qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable)
{
    return rustVideoPlaybackClampedSeekPosition(
        currentPosition, deltaMilliseconds, duration, seekable);
}
}
