// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTCOMMANDRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONVIDEODOCUMENTCOMMANDRUNTIME_H

#include "session/documentsessiondocumentports.h"
#include "session/documentsessionvideooutputruntime.h"

#include <QUrl>
#include <functional>

namespace kiriview {
using DocumentSessionVideoOutputClearPort
    = std::function<void(const DocumentSessionVideoOutputAttachmentPort&)>;

class DocumentSessionVideoDocumentCommandRuntime final
{
public:
    explicit DocumentSessionVideoDocumentCommandRuntime(
        DocumentSessionVideoDocumentCommandPort commands = {},
        DocumentSessionVideoOutputClearPort clearVideoOutput = {});

    void setSourceUrl(const QUrl& sourceUrl);
    void leaveMode(const QUrl& currentSourceUrl);
    DocumentSessionVideoOutputAttachmentPort outputAttachmentPort() const;

private:
    QObject* videoOutput() const;
    void clearVideoOutput();

    DocumentSessionVideoDocumentCommandPort m_commands;
    DocumentSessionVideoOutputClearPort m_clearVideoOutput;
};
}

#endif
