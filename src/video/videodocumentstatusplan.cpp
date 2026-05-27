// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstatusplan.h"

namespace {
KiriView::VideoDocumentStatus documentStatusForMediaStatus(KiriView::VideoMediaStatus status)
{
    switch (status) {
    case KiriView::VideoMediaStatus::Null:
        return KiriView::VideoDocumentStatus::Loading;
    case KiriView::VideoMediaStatus::Loading:
    case KiriView::VideoMediaStatus::Stalled:
        return KiriView::VideoDocumentStatus::Loading;
    case KiriView::VideoMediaStatus::Loaded:
    case KiriView::VideoMediaStatus::Buffering:
    case KiriView::VideoMediaStatus::Buffered:
    case KiriView::VideoMediaStatus::EndOfMedia:
        return KiriView::VideoDocumentStatus::Ready;
    case KiriView::VideoMediaStatus::Invalid:
        return KiriView::VideoDocumentStatus::Error;
    }

    return KiriView::VideoDocumentStatus::Loading;
}
}

namespace KiriView {
VideoDocumentStatusPlan videoDocumentStatusPlan(VideoDocumentStatusSnapshot snapshot)
{
    if (snapshot.sourceUrlEmpty) {
        return VideoDocumentStatusPlan {
            VideoDocumentStatus::Null,
            false,
            false,
        };
    }

    if (snapshot.sourceLoadActive || !snapshot.mediaBackendAvailable) {
        return VideoDocumentStatusPlan {
            VideoDocumentStatus::Loading,
            false,
            false,
        };
    }

    const bool mediaEnded = snapshot.mediaStatus == VideoMediaStatus::EndOfMedia;
    return VideoDocumentStatusPlan {
        documentStatusForMediaStatus(snapshot.mediaStatus),
        mediaEnded,
        mediaEnded,
    };
}
}
