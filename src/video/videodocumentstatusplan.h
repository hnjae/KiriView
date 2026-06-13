// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTSTATUSPLAN_H
#define KIRIVIEW_VIDEODOCUMENTSTATUSPLAN_H

#include "video/videodocumenttypes.h"
#include "video/videomediabackend.h"

namespace kiriview {
struct VideoDocumentStatusSnapshot {
    bool sourceUrlEmpty = true;
    bool sourceLoadActive = false;
    bool mediaBackendAvailable = false;
    VideoMediaStatus mediaStatus = VideoMediaStatus::Null;
};

struct VideoDocumentStatusPlan {
    VideoDocumentStatus status = VideoDocumentStatus::Null;
    bool mediaEnded = false;
    bool clearPlaying = false;
};

VideoDocumentStatusPlan videoDocumentStatusPlan(VideoDocumentStatusSnapshot snapshot);
}

#endif
