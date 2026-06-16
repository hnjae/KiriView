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
    if (m_commands.setSourceUrl) {
        m_commands.setSourceUrl(sourceUrl);
    }
}

void DocumentSessionImageDocumentCommandRuntime::clearSourceUrl() { setSourceUrl(QUrl()); }

void DocumentSessionImageDocumentCommandRuntime::openPreviousPage()
{
    if (m_commands.openPreviousPage) {
        m_commands.openPreviousPage();
    }
}

void DocumentSessionImageDocumentCommandRuntime::openNextPage()
{
    if (m_commands.openNextPage) {
        m_commands.openNextPage();
    }
}

void DocumentSessionImageDocumentCommandRuntime::openImageAtPage(int number)
{
    if (m_commands.openImageAtPage) {
        m_commands.openImageAtPage(number);
    }
}

void DocumentSessionImageDocumentCommandRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    if (m_commands.deleteDisplayedFile) {
        m_commands.deleteDisplayedFile(mode);
    }
}
}
