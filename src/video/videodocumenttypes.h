// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTTYPES_H
#define KIRIVIEW_VIDEODOCUMENTTYPES_H

namespace KiriView {
enum class VideoDocumentStatus {
    Null,
    Loading,
    Ready,
    Error,
};

enum class VideoDocumentChange {
    SourceUrl,
    Status,
    ErrorString,
    WindowTitleFileName,
    Duration,
    Position,
    Playing,
    Seekable,
    HasVideo,
    HasAudio,
    ZoomPercentKnown,
    ZoomPercent,
    VideoOutput,
};
}

#endif
