// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirivideodocument.h"

#include "facade/videodocumentpublicsignals.h"
#include "video/videodocumentruntime.h"

#include <memory>

namespace {
KiriVideoDocument::Status fromVideoDocumentStatus(kiriview::VideoDocumentStatus status)
{
    switch (status) {
    case kiriview::VideoDocumentStatus::Null:
        return KiriVideoDocument::Status::Null;
    case kiriview::VideoDocumentStatus::Loading:
        return KiriVideoDocument::Status::Loading;
    case kiriview::VideoDocumentStatus::Ready:
        return KiriVideoDocument::Status::Ready;
    case kiriview::VideoDocumentStatus::Error:
        return KiriVideoDocument::Status::Error;
    }

    return KiriVideoDocument::Status::Null;
}

kiriview::VideoDocumentPublicSignalOperations publicSignalOperations(KiriVideoDocument &document)
{
    kiriview::VideoDocumentPublicSignalOperations operations;
    operations.sourceUrlChanged = [&document]() { Q_EMIT document.sourceUrlChanged(); };
    operations.statusChanged = [&document]() { Q_EMIT document.statusChanged(); };
    operations.errorStringChanged = [&document]() { Q_EMIT document.errorStringChanged(); };
    operations.windowTitleFileNameChanged
        = [&document]() { Q_EMIT document.windowTitleFileNameChanged(); };
    operations.durationChanged = [&document]() { Q_EMIT document.durationChanged(); };
    operations.positionChanged = [&document]() { Q_EMIT document.positionChanged(); };
    operations.playingChanged = [&document]() { Q_EMIT document.playingChanged(); };
    operations.seekableChanged = [&document]() { Q_EMIT document.seekableChanged(); };
    operations.hasVideoChanged = [&document]() { Q_EMIT document.hasVideoChanged(); };
    operations.hasAudioChanged = [&document]() { Q_EMIT document.hasAudioChanged(); };
    operations.videoSizeChanged = [&document]() { Q_EMIT document.videoSizeChanged(); };
    operations.zoomPercentKnownChanged
        = [&document]() { Q_EMIT document.zoomPercentKnownChanged(); };
    operations.zoomPercentChanged = [&document]() { Q_EMIT document.zoomPercentChanged(); };
    operations.mutedChanged = [&document]() { Q_EMIT document.mutedChanged(); };
    operations.videoOutputChanged = [&document]() { Q_EMIT document.videoOutputChanged(); };
    operations.embeddedMetadataChanged
        = [&document]() { Q_EMIT document.embeddedMetadataChanged(); };
    return operations;
}
}

KiriVideoDocument::KiriVideoDocument(QObject *parent)
    : QObject(parent)
{
    m_runtime = std::make_unique<kiriview::VideoDocumentRuntime>(
        this, [this](const std::vector<kiriview::VideoDocumentChange> &changes) {
            handleDocumentChanges(changes);
        });
}

KiriVideoDocument::~KiriVideoDocument() = default;

QUrl KiriVideoDocument::sourceUrl() const { return m_runtime->sourceUrl(); }

void KiriVideoDocument::setSourceUrl(const QUrl &sourceUrl) { m_runtime->setSourceUrl(sourceUrl); }

KiriVideoDocument::Status KiriVideoDocument::status() const
{
    return fromVideoDocumentStatus(m_runtime->status());
}

QString KiriVideoDocument::errorString() const { return m_runtime->errorString(); }

QString KiriVideoDocument::windowTitleFileName() const { return m_runtime->windowTitleFileName(); }

qint64 KiriVideoDocument::duration() const { return m_runtime->duration(); }

qint64 KiriVideoDocument::position() const { return m_runtime->position(); }

bool KiriVideoDocument::playing() const { return m_runtime->playing(); }

bool KiriVideoDocument::seekable() const { return m_runtime->seekable(); }

bool KiriVideoDocument::hasVideo() const { return m_runtime->hasVideo(); }

bool KiriVideoDocument::hasAudio() const { return m_runtime->hasAudio(); }

QSize KiriVideoDocument::videoSize() const { return m_runtime->videoSize(); }

bool KiriVideoDocument::zoomPercentKnown() const { return m_runtime->zoomPercentKnown(); }

int KiriVideoDocument::zoomPercent() const { return m_runtime->zoomPercent(); }

bool KiriVideoDocument::muted() const { return m_runtime->muted(); }

QObject *KiriVideoDocument::videoOutput() const { return m_runtime->videoOutput(); }

const kiriview::EmbeddedMetadata &KiriVideoDocument::embeddedMetadata() const
{
    return m_runtime->embeddedMetadata();
}

void KiriVideoDocument::setVideoOutput(QObject *videoOutput)
{
    m_runtime->setVideoOutput(videoOutput);
}

void KiriVideoDocument::play() { m_runtime->play(); }

void KiriVideoDocument::pause() { m_runtime->pause(); }

void KiriVideoDocument::stop() { m_runtime->stop(); }

void KiriVideoDocument::togglePlayback() { m_runtime->togglePlayback(); }

void KiriVideoDocument::setMuted(bool muted) { m_runtime->setMuted(muted); }

void KiriVideoDocument::toggleMuted() { m_runtime->toggleMuted(); }

void KiriVideoDocument::setPosition(qint64 position) { m_runtime->setPosition(position); }

void KiriVideoDocument::seekBy(qint64 deltaMilliseconds) { m_runtime->seekBy(deltaMilliseconds); }

void KiriVideoDocument::setVideoOutputGeometry(const QRectF &contentRect, const QRectF &sourceRect)
{
    m_runtime->setVideoOutputGeometry(contentRect, sourceRect);
}

void KiriVideoDocument::handleDocumentChanges(
    const std::vector<kiriview::VideoDocumentChange> &changes)
{
    kiriview::VideoDocumentPublicSignalEmitter(publicSignalOperations(*this)).emitChanges(changes);
}
