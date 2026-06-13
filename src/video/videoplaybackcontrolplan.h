// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEOPLAYBACKCONTROLPLAN_H
#define KIRIVIEW_VIDEOPLAYBACKCONTROLPLAN_H

#include <QtGlobal>
#include <optional>
#include <variant>
#include <vector>

namespace kiriview {
struct VideoPlaybackControlSnapshot {
    bool sourceUrlEmpty = true;
    bool mediaBackendAvailable = false;
    bool playing = false;
    bool mediaEnded = false;
    bool seekable = false;
    qint64 position = 0;
    qint64 duration = 0;
};

struct VideoPlaybackStateDelta {
    std::optional<bool> mediaEnded;
    std::optional<bool> playing;
    std::optional<qint64> position;
};

struct EnsureVideoPlaybackBackendOperation {
};

struct PlayVideoPlaybackOperation {
};

struct PauseVideoPlaybackOperation {
};

struct StopVideoPlaybackOperation {
};

struct SetVideoPlaybackPositionOperation {
    qint64 position = 0;
};

using VideoPlaybackBackendOperation
    = std::variant<EnsureVideoPlaybackBackendOperation, PlayVideoPlaybackOperation,
        PauseVideoPlaybackOperation, StopVideoPlaybackOperation, SetVideoPlaybackPositionOperation>;

struct VideoPlaybackControlPlan {
    VideoPlaybackStateDelta stateDelta;
    std::vector<VideoPlaybackBackendOperation> backendOperations;

    bool isEmpty() const;
};

VideoPlaybackControlPlan videoPlaybackPlayPlan(VideoPlaybackControlSnapshot snapshot);
VideoPlaybackControlPlan videoPlaybackPausePlan(VideoPlaybackControlSnapshot snapshot);
VideoPlaybackControlPlan videoPlaybackStopPlan(VideoPlaybackControlSnapshot snapshot);
VideoPlaybackControlPlan videoPlaybackTogglePlan(VideoPlaybackControlSnapshot snapshot);
VideoPlaybackControlPlan videoPlaybackSetPositionPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 position);
VideoPlaybackControlPlan videoPlaybackSeekByPlan(
    VideoPlaybackControlSnapshot snapshot, qint64 deltaMilliseconds);
qint64 videoPlaybackClampedSeekPosition(
    qint64 currentPosition, qint64 deltaMilliseconds, qint64 duration, bool seekable);
}

#endif
