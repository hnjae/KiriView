// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionimagedocumentcommandruntime.h"

#include <utility>

namespace kiriview {
DocumentSessionImageDocumentCommandRuntime::DocumentSessionImageDocumentCommandRuntime(
    DocumentSessionImageDocumentCommandPort commands)
    : m_commands(std::move(commands))
{
}

void DocumentSessionImageDocumentCommandRuntime::setSourceUrl(const QUrl &sourceUrl)
{
    if (m_commands.source.setSourceUrl) {
        m_commands.source.setSourceUrl(sourceUrl);
    }
}

void DocumentSessionImageDocumentCommandRuntime::clearSourceUrl() { setSourceUrl(QUrl()); }

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
