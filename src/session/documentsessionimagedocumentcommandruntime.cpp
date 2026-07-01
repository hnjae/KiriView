// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionimagedocumentcommandruntime.h"

#include <QString>
#include <utility>

namespace kiriview {
DocumentSessionImageDocumentCommandRuntime::DocumentSessionImageDocumentCommandRuntime(
    DocumentSessionImageDocumentCommandPort commands)
    : m_commands(std::move(commands))
{
}

void DocumentSessionImageDocumentCommandRuntime::setSourceUrl(const QUrl& sourceUrl)
{
    if (m_commands.source.setSourceUrl) {
        m_commands.source.setSourceUrl(sourceUrl);
    }
}

void DocumentSessionImageDocumentCommandRuntime::setSameScopeImageNavigationSourceUrl(
    const QUrl& sourceUrl)
{
    if (m_commands.source.setSameScopeImageNavigationSourceUrl) {
        m_commands.source.setSameScopeImageNavigationSourceUrl(sourceUrl);
    }
}

void DocumentSessionImageDocumentCommandRuntime::clearSourceUrl() { setSourceUrl(QUrl()); }

MediaEntrySourceVideoPlaybackDeviceResult
DocumentSessionImageDocumentCommandRuntime::loadOpenedCollectionVideoPlaybackDevice(
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& videoUrl)
{
    if (m_commands.source.loadOpenedCollectionVideoPlaybackDevice) {
        return m_commands.source.loadOpenedCollectionVideoPlaybackDevice(
            openedCollectionScope, videoUrl);
    }

    return MediaEntrySourceError {
        MediaEntrySourceBackendKind::Unsupported,
        MediaEntrySourceOperation::OpenVideoPlaybackDevice,
        openedCollectionScope.fileUrl(),
        QString(),
        QString(),
        QString(),
    };
}

void DocumentSessionImageDocumentCommandRuntime::openPreviousPage()
{
    if (m_commands.pageNavigation.openPreviousPage) {
        m_commands.pageNavigation.openPreviousPage();
    }
}

void DocumentSessionImageDocumentCommandRuntime::openNextPage()
{
    if (m_commands.pageNavigation.openNextPage) {
        m_commands.pageNavigation.openNextPage();
    }
}

void DocumentSessionImageDocumentCommandRuntime::openImageAtPage(int number)
{
    if (m_commands.pageNavigation.openImageAtPage) {
        m_commands.pageNavigation.openImageAtPage(number);
    }
}

void DocumentSessionImageDocumentCommandRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_commands.deletion.deleteDisplayedFile) {
        m_commands.deletion.deleteDisplayedFile(mode);
    }
}
}
