// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "videodocumentpublicsignals.h"

#include <algorithm>
#include <utility>

namespace {
template <typename Operation> void run(const Operation &operation)
{
    if (operation) {
        operation();
    }
}
}

namespace KiriView {
VideoDocumentPublicSignalEmitter::VideoDocumentPublicSignalEmitter(
    VideoDocumentPublicSignalOperations operations)
    : m_operations(std::move(operations))
{
}

void VideoDocumentPublicSignalEmitter::emitChanges(
    const std::vector<VideoDocumentChange> &changes) const
{
    for (VideoDocumentPublicSignal signal : videoDocumentPublicSignalsForChanges(changes)) {
        emitSignal(signal);
    }
}

void VideoDocumentPublicSignalEmitter::emitSignal(VideoDocumentPublicSignal signal) const
{
    switch (signal) {
    case VideoDocumentPublicSignal::SourceUrl:
        run(m_operations.sourceUrlChanged);
        return;
    case VideoDocumentPublicSignal::Status:
        run(m_operations.statusChanged);
        return;
    case VideoDocumentPublicSignal::ErrorString:
        run(m_operations.errorStringChanged);
        return;
    case VideoDocumentPublicSignal::WindowTitleFileName:
        run(m_operations.windowTitleFileNameChanged);
        return;
    case VideoDocumentPublicSignal::Duration:
        run(m_operations.durationChanged);
        return;
    case VideoDocumentPublicSignal::Position:
        run(m_operations.positionChanged);
        return;
    case VideoDocumentPublicSignal::Playing:
        run(m_operations.playingChanged);
        return;
    case VideoDocumentPublicSignal::Seekable:
        run(m_operations.seekableChanged);
        return;
    case VideoDocumentPublicSignal::HasVideo:
        run(m_operations.hasVideoChanged);
        return;
    case VideoDocumentPublicSignal::HasAudio:
        run(m_operations.hasAudioChanged);
        return;
    case VideoDocumentPublicSignal::VideoOutput:
        run(m_operations.videoOutputChanged);
        return;
    }
}

std::vector<VideoDocumentPublicSignal> videoDocumentPublicSignals(VideoDocumentChange change)
{
    switch (change) {
    case VideoDocumentChange::SourceUrl:
        return { VideoDocumentPublicSignal::SourceUrl };
    case VideoDocumentChange::Status:
        return { VideoDocumentPublicSignal::Status };
    case VideoDocumentChange::ErrorString:
        return { VideoDocumentPublicSignal::ErrorString };
    case VideoDocumentChange::WindowTitleFileName:
        return { VideoDocumentPublicSignal::WindowTitleFileName };
    case VideoDocumentChange::Duration:
        return { VideoDocumentPublicSignal::Duration };
    case VideoDocumentChange::Position:
        return { VideoDocumentPublicSignal::Position };
    case VideoDocumentChange::Playing:
        return { VideoDocumentPublicSignal::Playing };
    case VideoDocumentChange::Seekable:
        return { VideoDocumentPublicSignal::Seekable };
    case VideoDocumentChange::HasVideo:
        return { VideoDocumentPublicSignal::HasVideo };
    case VideoDocumentChange::HasAudio:
        return { VideoDocumentPublicSignal::HasAudio };
    case VideoDocumentChange::VideoOutput:
        return { VideoDocumentPublicSignal::VideoOutput };
    }

    return {};
}

std::vector<VideoDocumentPublicSignal> videoDocumentPublicSignalsForChanges(
    const std::vector<VideoDocumentChange> &changes)
{
    std::vector<VideoDocumentPublicSignal> plannedSignals;
    for (VideoDocumentChange change : changes) {
        for (VideoDocumentPublicSignal signal : videoDocumentPublicSignals(change)) {
            const bool alreadyPlanned
                = std::find(plannedSignals.cbegin(), plannedSignals.cend(), signal)
                != plannedSignals.cend();
            if (!alreadyPlanned) {
                plannedSignals.push_back(signal);
            }
        }
    }
    return plannedSignals;
}
}
