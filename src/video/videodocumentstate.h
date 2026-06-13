// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_VIDEODOCUMENTSTATE_H
#define KIRIVIEW_VIDEODOCUMENTSTATE_H

#include "video/videodocumenttypes.h"

#include "metadata/embeddedmetadata.h"

#include <QSize>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <optional>
#include <vector>

namespace kiriview {
class VideoDocumentState final
{
public:
    using ChangeCallback = std::function<void(const std::vector<VideoDocumentChange> &)>;

    explicit VideoDocumentState(ChangeCallback changeCallback = {});

    const QUrl &sourceUrl() const;
    VideoDocumentStatus status() const;
    const QString &errorString() const;
    const QString &windowTitleFileName() const;
    qint64 duration() const;
    qint64 position() const;
    bool playing() const;
    bool seekable() const;
    bool hasVideo() const;
    bool hasAudio() const;
    QSize videoSize() const;
    bool zoomPercentKnown() const;
    int zoomPercent() const;
    bool muted() const;
    bool mediaEnded() const;
    const EmbeddedMetadata &embeddedMetadata() const;

    void resetForClearedSource();
    void resetForSourceLoad(const QUrl &sourceUrl);
    void setStatusAndError(VideoDocumentStatus status, const QString &errorString = {});
    void setStatus(VideoDocumentStatus status);
    void setErrorString(const QString &errorString);
    void setDuration(qint64 duration);
    void setPosition(qint64 position);
    void setPlaying(bool playing);
    void setSeekable(bool seekable);
    void setHasVideo(bool hasVideo);
    void setHasAudio(bool hasAudio);
    void setVideoSize(QSize size);
    void setZoomPercent(std::optional<int> zoomPercent);
    void setMuted(bool muted);
    void setMediaEnded(bool mediaEnded);
    void setEmbeddedMetadata(EmbeddedMetadata metadata);

    void publish(VideoDocumentChange change);
    void publish(std::vector<VideoDocumentChange> changes);

private:
    void appendIfSourceUrlChanged(std::vector<VideoDocumentChange> &changes, const QUrl &sourceUrl);
    void appendIfStatusChanged(
        std::vector<VideoDocumentChange> &changes, VideoDocumentStatus status);
    void appendIfErrorStringChanged(
        std::vector<VideoDocumentChange> &changes, const QString &errorString);
    void appendIfWindowTitleFileNameChanged(
        std::vector<VideoDocumentChange> &changes, const QString &fileName);
    void appendIfDurationChanged(std::vector<VideoDocumentChange> &changes, qint64 duration);
    void appendIfPositionChanged(std::vector<VideoDocumentChange> &changes, qint64 position);
    void appendIfPlayingChanged(std::vector<VideoDocumentChange> &changes, bool playing);
    void appendIfSeekableChanged(std::vector<VideoDocumentChange> &changes, bool seekable);
    void appendIfHasVideoChanged(std::vector<VideoDocumentChange> &changes, bool hasVideo);
    void appendIfHasAudioChanged(std::vector<VideoDocumentChange> &changes, bool hasAudio);
    void appendIfVideoSizeChanged(std::vector<VideoDocumentChange> &changes, QSize size);
    void appendIfZoomPercentKnownChanged(std::vector<VideoDocumentChange> &changes, bool known);
    void appendIfZoomPercentChanged(std::vector<VideoDocumentChange> &changes, int zoomPercent);
    void appendIfMutedChanged(std::vector<VideoDocumentChange> &changes, bool muted);

    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    VideoDocumentStatus m_status = VideoDocumentStatus::Null;
    QString m_errorString;
    QString m_windowTitleFileName;
    qint64 m_duration = 0;
    qint64 m_position = 0;
    bool m_playing = false;
    bool m_seekable = false;
    bool m_hasVideo = false;
    bool m_hasAudio = false;
    QSize m_videoSize;
    bool m_zoomPercentKnown = false;
    int m_zoomPercent = 0;
    bool m_muted = false;
    bool m_mediaEnded = false;
    EmbeddedMetadata m_embeddedMetadata;
};
}

#endif
