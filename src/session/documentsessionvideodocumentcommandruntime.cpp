// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionvideodocumentcommandruntime.h"

#include <QObject>
#include <utility>

namespace kiriview {
DocumentSessionVideoDocumentCommandRuntime::DocumentSessionVideoDocumentCommandRuntime(
    DocumentSessionVideoDocumentCommandPort commands,
    DocumentSessionVideoOutputClearPort clearVideoOutput)
    : m_commands(std::move(commands))
    , m_clearVideoOutput(std::move(clearVideoOutput))
{
}

void DocumentSessionVideoDocumentCommandRuntime::setSourceUrl(const QUrl& sourceUrl)
{
    if (m_commands.source.setSourceUrl) {
        m_commands.source.setSourceUrl(sourceUrl);
    }
}

void DocumentSessionVideoDocumentCommandRuntime::setSourceDevice(
    const QUrl& sourceUrl, VideoPlaybackSourceDevice sourceDevice)
{
    if (m_commands.source.setSourceDevice) {
        m_commands.source.setSourceDevice(sourceUrl, std::move(sourceDevice));
    }
}

void DocumentSessionVideoDocumentCommandRuntime::leaveMode(const QUrl& currentSourceUrl)
{
    if (currentSourceUrl.isEmpty() && videoOutput() == nullptr) {
        return;
    }

    clearVideoOutput();
    if (m_commands.playback.stop) {
        m_commands.playback.stop();
    }
    setSourceUrl(QUrl());
}

DocumentSessionVideoOutputAttachmentPort
DocumentSessionVideoDocumentCommandRuntime::outputAttachmentPort() const
{
    return DocumentSessionVideoOutputAttachmentPort {
        [this](QObject* videoOutput) {
            if (m_commands.output.setVideoOutput) {
                m_commands.output.setVideoOutput(videoOutput);
            }
        },
        [this](const QRectF& contentRect, const QRectF& sourceRect) {
            if (m_commands.output.setVideoOutputGeometry) {
                m_commands.output.setVideoOutputGeometry(contentRect, sourceRect);
            }
        },
    };
}

QObject* DocumentSessionVideoDocumentCommandRuntime::videoOutput() const
{
    if (!m_commands.output.videoOutput) {
        return nullptr;
    }

    return m_commands.output.videoOutput();
}

void DocumentSessionVideoDocumentCommandRuntime::clearVideoOutput()
{
    const DocumentSessionVideoOutputAttachmentPort attachmentPort = outputAttachmentPort();
    if (m_clearVideoOutput) {
        m_clearVideoOutput(attachmentPort);
        return;
    }

    if (attachmentPort.setVideoOutput) {
        attachmentPort.setVideoOutput(nullptr);
    }
}
}
