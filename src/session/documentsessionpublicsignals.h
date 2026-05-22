// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICSIGNALS_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICSIGNALS_H

#include "documentsessionstate.h"

#include <functional>
#include <vector>

namespace KiriView {
enum class DocumentSessionPublicSignal {
    SourceUrl,
    DocumentKind,
    ErrorString,
    WindowTitleFileName,
    DisplayedFileDeletionAvailability,
    FileDeletionInProgress,
    MediaNavigationAvailability,
};

struct DocumentSessionPublicSignalOperations {
    std::function<void()> sourceUrlChanged;
    std::function<void()> documentKindChanged;
    std::function<void()> errorStringChanged;
    std::function<void()> windowTitleFileNameChanged;
    std::function<void()> displayedFileDeletionAvailabilityChanged;
    std::function<void()> fileDeletionInProgressChanged;
    std::function<void()> mediaNavigationAvailabilityChanged;
};

class DocumentSessionPublicSignalEmitter final
{
public:
    explicit DocumentSessionPublicSignalEmitter(DocumentSessionPublicSignalOperations operations);

    void emitChanges(const std::vector<DocumentSessionChange> &changes) const;
    void emitSignal(DocumentSessionPublicSignal signal) const;

private:
    DocumentSessionPublicSignalOperations m_operations;
};

std::vector<DocumentSessionPublicSignal> documentSessionPublicSignals(DocumentSessionChange change);
std::vector<DocumentSessionPublicSignal> documentSessionPublicSignalsForChanges(
    const std::vector<DocumentSessionChange> &changes);
}

#endif
