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

namespace kiriview {
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
    case DocumentSessionPublicSignal::PublicProjectionRevision:
        run(m_operations.publicProjectionRevisionChanged);
        return;
    case DocumentSessionPublicSignal::SourceUrl:
        run(m_operations.sourceUrlChanged);
        return;
    case DocumentSessionPublicSignal::DocumentKind:
        run(m_operations.documentKindChanged);
        return;
    case DocumentSessionPublicSignal::ErrorString:
        run(m_operations.errorStringChanged);
        return;
    case DocumentSessionPublicSignal::WindowTitleSubject:
        run(m_operations.windowTitleSubjectChanged);
        return;
    case DocumentSessionPublicSignal::ActiveZoomReadout:
        run(m_operations.activeZoomReadoutChanged);
        return;
    case DocumentSessionPublicSignal::ActiveMediaReadiness:
        run(m_operations.activeMediaReadinessChanged);
        return;
    case DocumentSessionPublicSignal::DisplayedMediaOpenWithAvailability:
        run(m_operations.displayedMediaOpenWithAvailabilityChanged);
        return;
    case DocumentSessionPublicSignal::DisplayedFileDeletionAvailability:
        run(m_operations.displayedFileDeletionAvailabilityChanged);
        return;
    case DocumentSessionPublicSignal::FileDeletionInProgress:
        run(m_operations.fileDeletionInProgressChanged);
        return;
    case DocumentSessionPublicSignal::ActiveNavigation:
        run(m_operations.activeNavigationChanged);
        return;
    case DocumentSessionPublicSignal::ActiveNavigationRevealIntent:
        run(m_operations.activeNavigationRevealIntentChanged);
        return;
    case DocumentSessionPublicSignal::ActiveNavigationRevealDirection:
        run(m_operations.activeNavigationRevealDirectionChanged);
        return;
    }
}

std::vector<DocumentSessionPublicSignal> documentSessionPublicSignals(DocumentSessionChange change)
{
    switch (change) {
    case DocumentSessionChange::PublicProjectionRevision:
        return { DocumentSessionPublicSignal::PublicProjectionRevision };
    case DocumentSessionChange::SourceUrl:
        return { DocumentSessionPublicSignal::SourceUrl };
    case DocumentSessionChange::DocumentKind:
        return { DocumentSessionPublicSignal::DocumentKind };
    case DocumentSessionChange::ErrorString:
        return { DocumentSessionPublicSignal::ErrorString };
    case DocumentSessionChange::WindowTitleSubject:
        return { DocumentSessionPublicSignal::WindowTitleSubject };
    case DocumentSessionChange::ActiveZoomReadout:
        return { DocumentSessionPublicSignal::ActiveZoomReadout };
    case DocumentSessionChange::ActiveMediaReadiness:
        return { DocumentSessionPublicSignal::ActiveMediaReadiness };
    case DocumentSessionChange::OpenWithAvailability:
        return { DocumentSessionPublicSignal::DisplayedMediaOpenWithAvailability };
    case DocumentSessionChange::FileDeletionAvailability:
        return { DocumentSessionPublicSignal::DisplayedFileDeletionAvailability };
    case DocumentSessionChange::FileDeletionInProgress:
        return { DocumentSessionPublicSignal::FileDeletionInProgress };
    case DocumentSessionChange::ActiveNavigation:
        return { DocumentSessionPublicSignal::ActiveNavigation };
    case DocumentSessionChange::ActiveNavigationRevealIntent:
        return { DocumentSessionPublicSignal::ActiveNavigationRevealIntent };
    case DocumentSessionChange::ActiveNavigationRevealDirection:
        return { DocumentSessionPublicSignal::ActiveNavigationRevealDirection };
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
