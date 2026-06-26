// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTCOMMANDRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONIMAGEDOCUMENTCOMMANDRUNTIME_H

#include "session/documentsessiondocumentports.h"

#include <QUrl>

namespace kiriview {
class DocumentSessionImageDocumentCommandRuntime final
{
public:
    explicit DocumentSessionImageDocumentCommandRuntime(
        DocumentSessionImageDocumentCommandPort commands = {});

    void setSourceUrl(const QUrl& sourceUrl);
    void clearSourceUrl();
    void openPreviousPage();
    void openNextPage();
    void openImageAtPage(int number);
    void deleteDisplayedFile(FileDeletionMode mode);

private:
    DocumentSessionImageDocumentCommandPort m_commands;
};
}

#endif
