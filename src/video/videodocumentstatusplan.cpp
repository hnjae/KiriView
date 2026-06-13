// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstatusplan.h"

#include "bridge/videodocumentpolicyconversion.h"
#include "kiriview/src/policy/videodocumentpolicy.cxx.h"

namespace kiriview {
VideoDocumentStatusPlan videoDocumentStatusPlan(VideoDocumentStatusSnapshot snapshot)
{
    return Bridge::videoDocumentStatusPlanFromRust(
        rustVideoDocumentStatusPlan(Bridge::rustVideoDocumentStatusSnapshot(snapshot)));
}
}
