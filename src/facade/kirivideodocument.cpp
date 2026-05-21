// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirivideodocument.h"

#include "video/videodocumentruntime.h"

#include <memory>

namespace {
KiriVideoDocument::Status fromVideoDocumentStatus(KiriView::VideoDocumentStatus status)
{
    switch (status) {
    case KiriView::VideoDocumentStatus::Null:
        return KiriVideoDocument::Status::Null;
    case KiriView::VideoDocumentStatus::Loading:
        return KiriVideoDocument::Status::Loading;
    case KiriView::VideoDocumentStatus::Ready:
        return KiriVideoDocument::Status::Ready;
    case KiriView::VideoDocumentStatus::Error:
        return KiriVideoDocument::Status::Error;
    }

    return KiriVideoDocument::Status::Null;
}
}

KiriVideoDocument::KiriVideoDocument(QObject *parent)
    : QObject(parent)
{
    m_runtime = std::make_unique<KiriView::VideoDocumentRuntime>(
        this, [this](const std::vector<KiriView::VideoDocumentChange> &changes) {
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

QObject *KiriVideoDocument::videoOutput() const { return m_runtime->videoOutput(); }

void KiriVideoDocument::setVideoOutput(QObject *videoOutput)
{
    m_runtime->setVideoOutput(videoOutput);
}

void KiriVideoDocument::play() { m_runtime->play(); }

void KiriVideoDocument::pause() { m_runtime->pause(); }

void KiriVideoDocument::stop() { m_runtime->stop(); }

void KiriVideoDocument::togglePlayback() { m_runtime->togglePlayback(); }

void KiriVideoDocument::setPosition(qint64 position) { m_runtime->setPosition(position); }

void KiriVideoDocument::seekBy(qint64 deltaMilliseconds) { m_runtime->seekBy(deltaMilliseconds); }

void KiriVideoDocument::handleDocumentChanges(
    const std::vector<KiriView::VideoDocumentChange> &changes)
{
    for (KiriView::VideoDocumentChange change : changes) {
        switch (change) {
        case KiriView::VideoDocumentChange::SourceUrl:
            Q_EMIT sourceUrlChanged();
            break;
        case KiriView::VideoDocumentChange::Status:
            Q_EMIT statusChanged();
            break;
        case KiriView::VideoDocumentChange::ErrorString:
            Q_EMIT errorStringChanged();
            break;
        case KiriView::VideoDocumentChange::WindowTitleFileName:
            Q_EMIT windowTitleFileNameChanged();
            break;
        case KiriView::VideoDocumentChange::Duration:
            Q_EMIT durationChanged();
            break;
        case KiriView::VideoDocumentChange::Position:
            Q_EMIT positionChanged();
            break;
        case KiriView::VideoDocumentChange::Playing:
            Q_EMIT playingChanged();
            break;
        case KiriView::VideoDocumentChange::Seekable:
            Q_EMIT seekableChanged();
            break;
        case KiriView::VideoDocumentChange::HasVideo:
            Q_EMIT hasVideoChanged();
            break;
        case KiriView::VideoDocumentChange::HasAudio:
            Q_EMIT hasAudioChanged();
            break;
        case KiriView::VideoDocumentChange::VideoOutput:
            Q_EMIT videoOutputChanged();
            break;
        }
    }
}
