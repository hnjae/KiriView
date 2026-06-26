// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_VIDEODOCUMENTPOLICYCONVERSION_H
#define KIRIVIEW_BRIDGE_VIDEODOCUMENTPOLICYCONVERSION_H

#include "kiriview/src/policy/videodocumentpolicy.cxx.h"
#include "video/videodocumentstatusplan.h"
#include "video/videoplaybackcontrolplan.h"

namespace kiriview::Bridge {
RustVideoDocumentStatusSnapshot rustVideoDocumentStatusSnapshot(
    const VideoDocumentStatusSnapshot& snapshot);
VideoDocumentStatusPlan videoDocumentStatusPlanFromRust(const RustVideoDocumentStatusPlan& plan);

RustVideoPlaybackControlSnapshot rustVideoPlaybackControlSnapshot(
    const VideoPlaybackControlSnapshot& snapshot);
VideoPlaybackControlPlan videoPlaybackControlPlanFromRust(const RustVideoPlaybackControlPlan& plan);
}

#endif
