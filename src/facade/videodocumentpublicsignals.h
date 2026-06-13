// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTPUBLICSIGNALS_H
#define KIRIVIEW_VIDEODOCUMENTPUBLICSIGNALS_H

#include "video/videodocumenttypes.h"

#include <functional>
#include <vector>

namespace kiriview {
enum class VideoDocumentPublicSignal {
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
    VideoSize,
    ZoomPercentKnown,
    ZoomPercent,
    Muted,
    VideoOutput,
    EmbeddedMetadata,
};

struct VideoDocumentPublicSignalOperations {
    std::function<void()> sourceUrlChanged;
    std::function<void()> statusChanged;
    std::function<void()> errorStringChanged;
    std::function<void()> windowTitleFileNameChanged;
    std::function<void()> durationChanged;
    std::function<void()> positionChanged;
    std::function<void()> playingChanged;
    std::function<void()> seekableChanged;
    std::function<void()> hasVideoChanged;
    std::function<void()> hasAudioChanged;
    std::function<void()> videoSizeChanged;
    std::function<void()> zoomPercentKnownChanged;
    std::function<void()> zoomPercentChanged;
    std::function<void()> mutedChanged;
    std::function<void()> videoOutputChanged;
    std::function<void()> embeddedMetadataChanged;
};

class VideoDocumentPublicSignalEmitter final
{
public:
    explicit VideoDocumentPublicSignalEmitter(VideoDocumentPublicSignalOperations operations);

    void emitChanges(const std::vector<VideoDocumentChange> &changes) const;
    void emitSignal(VideoDocumentPublicSignal signal) const;

private:
    VideoDocumentPublicSignalOperations m_operations;
};

std::vector<VideoDocumentPublicSignal> videoDocumentPublicSignals(VideoDocumentChange change);
std::vector<VideoDocumentPublicSignal> videoDocumentPublicSignalsForChanges(
    const std::vector<VideoDocumentChange> &changes);
}

#endif
