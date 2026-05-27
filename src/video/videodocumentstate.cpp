// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstate.h"

#include <algorithm>
#include <utility>

namespace {
QString fileNameForWindowTitle(const QUrl &sourceUrl)
{
    return sourceUrl.fileName(QUrl::PrettyDecoded);
}

qint64 nonNegative(qint64 value) { return std::max<qint64>(0, value); }

QSize normalizedVideoSize(QSize size)
{
    if (size.width() <= 0 || size.height() <= 0) {
        return {};
    }

    return size;
}
}

namespace KiriView {
VideoDocumentState::VideoDocumentState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

const QUrl &VideoDocumentState::sourceUrl() const { return m_sourceUrl; }

VideoDocumentStatus VideoDocumentState::status() const { return m_status; }

const QString &VideoDocumentState::errorString() const { return m_errorString; }

const QString &VideoDocumentState::windowTitleFileName() const { return m_windowTitleFileName; }

qint64 VideoDocumentState::duration() const { return m_duration; }

qint64 VideoDocumentState::position() const { return m_position; }

bool VideoDocumentState::playing() const { return m_playing; }

bool VideoDocumentState::seekable() const { return m_seekable; }

bool VideoDocumentState::hasVideo() const { return m_hasVideo; }

bool VideoDocumentState::hasAudio() const { return m_hasAudio; }

QSize VideoDocumentState::videoSize() const { return m_videoSize; }

bool VideoDocumentState::zoomPercentKnown() const { return m_zoomPercentKnown; }

int VideoDocumentState::zoomPercent() const { return m_zoomPercent; }

bool VideoDocumentState::mediaEnded() const { return m_mediaEnded; }

void VideoDocumentState::resetForClearedSource()
{
    m_mediaEnded = false;

    std::vector<VideoDocumentChange> changes;
    appendIfSourceUrlChanged(changes, QUrl());
    appendIfStatusChanged(changes, VideoDocumentStatus::Null);
    appendIfErrorStringChanged(changes, QString());
    appendIfWindowTitleFileNameChanged(changes, QString());
    appendIfDurationChanged(changes, 0);
    appendIfPositionChanged(changes, 0);
    appendIfPlayingChanged(changes, false);
    appendIfSeekableChanged(changes, false);
    appendIfHasVideoChanged(changes, false);
    appendIfHasAudioChanged(changes, false);
    appendIfVideoSizeChanged(changes, {});
    appendIfZoomPercentKnownChanged(changes, false);
    appendIfZoomPercentChanged(changes, 0);
    publish(std::move(changes));
}

void VideoDocumentState::resetForSourceLoad(const QUrl &sourceUrl)
{
    m_mediaEnded = false;

    std::vector<VideoDocumentChange> changes;
    appendIfSourceUrlChanged(changes, sourceUrl);
    appendIfWindowTitleFileNameChanged(changes, fileNameForWindowTitle(sourceUrl));
    appendIfErrorStringChanged(changes, QString());
    appendIfStatusChanged(changes, VideoDocumentStatus::Loading);
    appendIfDurationChanged(changes, 0);
    appendIfPositionChanged(changes, 0);
    appendIfPlayingChanged(changes, false);
    appendIfSeekableChanged(changes, false);
    appendIfHasVideoChanged(changes, false);
    appendIfHasAudioChanged(changes, false);
    appendIfVideoSizeChanged(changes, {});
    appendIfZoomPercentKnownChanged(changes, false);
    appendIfZoomPercentChanged(changes, 0);
    publish(std::move(changes));
}

void VideoDocumentState::setStatus(VideoDocumentStatus status)
{
    std::vector<VideoDocumentChange> changes;
    appendIfStatusChanged(changes, status);
    publish(std::move(changes));
}

void VideoDocumentState::setErrorString(const QString &errorString)
{
    std::vector<VideoDocumentChange> changes;
    appendIfErrorStringChanged(changes, errorString);
    publish(std::move(changes));
}

void VideoDocumentState::setDuration(qint64 duration)
{
    std::vector<VideoDocumentChange> changes;
    appendIfDurationChanged(changes, duration);
    publish(std::move(changes));
}

void VideoDocumentState::setPosition(qint64 position)
{
    std::vector<VideoDocumentChange> changes;
    appendIfPositionChanged(changes, position);
    publish(std::move(changes));
}

void VideoDocumentState::setPlaying(bool playing)
{
    if (playing) {
        m_mediaEnded = false;
    }

    std::vector<VideoDocumentChange> changes;
    appendIfPlayingChanged(changes, playing);
    publish(std::move(changes));
}

void VideoDocumentState::setSeekable(bool seekable)
{
    std::vector<VideoDocumentChange> changes;
    appendIfSeekableChanged(changes, seekable);
    publish(std::move(changes));
}

void VideoDocumentState::setHasVideo(bool hasVideo)
{
    std::vector<VideoDocumentChange> changes;
    appendIfHasVideoChanged(changes, hasVideo);
    publish(std::move(changes));
}

void VideoDocumentState::setHasAudio(bool hasAudio)
{
    std::vector<VideoDocumentChange> changes;
    appendIfHasAudioChanged(changes, hasAudio);
    publish(std::move(changes));
}

void VideoDocumentState::setVideoSize(QSize size)
{
    std::vector<VideoDocumentChange> changes;
    appendIfVideoSizeChanged(changes, size);
    publish(std::move(changes));
}

void VideoDocumentState::setZoomPercent(std::optional<int> zoomPercent)
{
    std::vector<VideoDocumentChange> changes;
    if (zoomPercent.has_value()) {
        appendIfZoomPercentChanged(changes, zoomPercent.value());
        appendIfZoomPercentKnownChanged(changes, true);
    } else {
        appendIfZoomPercentKnownChanged(changes, false);
        appendIfZoomPercentChanged(changes, 0);
    }
    publish(std::move(changes));
}

void VideoDocumentState::setMediaEnded(bool mediaEnded) { m_mediaEnded = mediaEnded; }

void VideoDocumentState::publish(VideoDocumentChange change)
{
    publish(std::vector<VideoDocumentChange> { change });
}

void VideoDocumentState::publish(std::vector<VideoDocumentChange> changes)
{
    if (changes.empty()) {
        return;
    }

    std::vector<VideoDocumentChange> uniqueChanges;
    for (VideoDocumentChange change : changes) {
        if (std::find(uniqueChanges.cbegin(), uniqueChanges.cend(), change)
            == uniqueChanges.cend()) {
            uniqueChanges.push_back(change);
        }
    }
    if (m_changeCallback) {
        m_changeCallback(uniqueChanges);
    }
}

void VideoDocumentState::appendIfSourceUrlChanged(
    std::vector<VideoDocumentChange> &changes, const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    m_sourceUrl = sourceUrl;
    changes.push_back(VideoDocumentChange::SourceUrl);
}

void VideoDocumentState::appendIfStatusChanged(
    std::vector<VideoDocumentChange> &changes, VideoDocumentStatus status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    changes.push_back(VideoDocumentChange::Status);
}

void VideoDocumentState::appendIfErrorStringChanged(
    std::vector<VideoDocumentChange> &changes, const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    changes.push_back(VideoDocumentChange::ErrorString);
}

void VideoDocumentState::appendIfWindowTitleFileNameChanged(
    std::vector<VideoDocumentChange> &changes, const QString &fileName)
{
    if (m_windowTitleFileName == fileName) {
        return;
    }

    m_windowTitleFileName = fileName;
    changes.push_back(VideoDocumentChange::WindowTitleFileName);
}

void VideoDocumentState::appendIfDurationChanged(
    std::vector<VideoDocumentChange> &changes, qint64 duration)
{
    const qint64 normalizedDuration = nonNegative(duration);
    if (m_duration == normalizedDuration) {
        return;
    }

    m_duration = normalizedDuration;
    changes.push_back(VideoDocumentChange::Duration);
}

void VideoDocumentState::appendIfPositionChanged(
    std::vector<VideoDocumentChange> &changes, qint64 position)
{
    const qint64 normalizedPosition = nonNegative(position);
    if (m_position == normalizedPosition) {
        return;
    }

    m_position = normalizedPosition;
    changes.push_back(VideoDocumentChange::Position);
}

void VideoDocumentState::appendIfPlayingChanged(
    std::vector<VideoDocumentChange> &changes, bool playing)
{
    if (m_playing == playing) {
        return;
    }

    m_playing = playing;
    changes.push_back(VideoDocumentChange::Playing);
}

void VideoDocumentState::appendIfSeekableChanged(
    std::vector<VideoDocumentChange> &changes, bool seekable)
{
    if (m_seekable == seekable) {
        return;
    }

    m_seekable = seekable;
    changes.push_back(VideoDocumentChange::Seekable);
}

void VideoDocumentState::appendIfHasVideoChanged(
    std::vector<VideoDocumentChange> &changes, bool hasVideo)
{
    if (m_hasVideo == hasVideo) {
        return;
    }

    m_hasVideo = hasVideo;
    changes.push_back(VideoDocumentChange::HasVideo);
}

void VideoDocumentState::appendIfHasAudioChanged(
    std::vector<VideoDocumentChange> &changes, bool hasAudio)
{
    if (m_hasAudio == hasAudio) {
        return;
    }

    m_hasAudio = hasAudio;
    changes.push_back(VideoDocumentChange::HasAudio);
}

void VideoDocumentState::appendIfVideoSizeChanged(
    std::vector<VideoDocumentChange> &changes, QSize size)
{
    const QSize normalizedSize = normalizedVideoSize(size);
    if (m_videoSize == normalizedSize) {
        return;
    }

    m_videoSize = normalizedSize;
    changes.push_back(VideoDocumentChange::VideoSize);
}

void VideoDocumentState::appendIfZoomPercentKnownChanged(
    std::vector<VideoDocumentChange> &changes, bool known)
{
    if (m_zoomPercentKnown == known) {
        return;
    }

    m_zoomPercentKnown = known;
    changes.push_back(VideoDocumentChange::ZoomPercentKnown);
}

void VideoDocumentState::appendIfZoomPercentChanged(
    std::vector<VideoDocumentChange> &changes, int zoomPercent)
{
    const int normalizedZoomPercent = nonNegative(zoomPercent);
    if (m_zoomPercent == normalizedZoomPercent) {
        return;
    }

    m_zoomPercent = normalizedZoomPercent;
    changes.push_back(VideoDocumentChange::ZoomPercent);
}
}
