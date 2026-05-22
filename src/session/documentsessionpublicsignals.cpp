// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionpublicsignals.h"

#include <algorithm>
#include <utility>

namespace {
template <typename Operation> void run(const Operation &operation)
{
    if (operation) {
        operation();
    }
}
}

namespace KiriView {
DocumentSessionPublicSignalEmitter::DocumentSessionPublicSignalEmitter(
    DocumentSessionPublicSignalOperations operations)
    : m_operations(std::move(operations))
{
}

void DocumentSessionPublicSignalEmitter::emitChanges(
    const std::vector<DocumentSessionChange> &changes) const
{
    for (DocumentSessionPublicSignal signal : documentSessionPublicSignalsForChanges(changes)) {
        emitSignal(signal);
    }
}

void DocumentSessionPublicSignalEmitter::emitSignal(DocumentSessionPublicSignal signal) const
{
    switch (signal) {
    case DocumentSessionPublicSignal::SourceUrl:
        run(m_operations.sourceUrlChanged);
        return;
    case DocumentSessionPublicSignal::DocumentKind:
        run(m_operations.documentKindChanged);
        return;
    case DocumentSessionPublicSignal::ErrorString:
        run(m_operations.errorStringChanged);
        return;
    case DocumentSessionPublicSignal::WindowTitleFileName:
        run(m_operations.windowTitleFileNameChanged);
        return;
    case DocumentSessionPublicSignal::DisplayedFileDeletionAvailability:
        run(m_operations.displayedFileDeletionAvailabilityChanged);
        return;
    case DocumentSessionPublicSignal::FileDeletionInProgress:
        run(m_operations.fileDeletionInProgressChanged);
        return;
    case DocumentSessionPublicSignal::MediaNavigationAvailability:
        run(m_operations.mediaNavigationAvailabilityChanged);
        return;
    }
}

std::vector<DocumentSessionPublicSignal> documentSessionPublicSignals(DocumentSessionChange change)
{
    switch (change) {
    case DocumentSessionChange::SourceUrl:
        return { DocumentSessionPublicSignal::SourceUrl };
    case DocumentSessionChange::DocumentKind:
        return { DocumentSessionPublicSignal::DocumentKind };
    case DocumentSessionChange::ErrorString:
        return { DocumentSessionPublicSignal::ErrorString };
    case DocumentSessionChange::WindowTitleFileName:
        return { DocumentSessionPublicSignal::WindowTitleFileName };
    case DocumentSessionChange::FileDeletionAvailability:
        return { DocumentSessionPublicSignal::DisplayedFileDeletionAvailability };
    case DocumentSessionChange::FileDeletionInProgress:
        return { DocumentSessionPublicSignal::FileDeletionInProgress };
    case DocumentSessionChange::MediaNavigationAvailability:
        return { DocumentSessionPublicSignal::MediaNavigationAvailability };
    }

    return {};
}

std::vector<DocumentSessionPublicSignal> documentSessionPublicSignalsForChanges(
    const std::vector<DocumentSessionChange> &changes)
{
    std::vector<DocumentSessionPublicSignal> plannedSignals;
    for (DocumentSessionChange change : changes) {
        for (DocumentSessionPublicSignal signal : documentSessionPublicSignals(change)) {
            const bool alreadyPlanned
                = std::find(plannedSignals.cbegin(), plannedSignals.cend(), signal)
                != plannedSignals.cend();
            if (!alreadyPlanned) {
                plannedSignals.push_back(signal);
            }
        }
    }
    return plannedSignals;
}
}
