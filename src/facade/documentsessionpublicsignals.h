// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICSIGNALS_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICSIGNALS_H

#include "session/documentsessiontypes.h"

#include <functional>
#include <vector>

namespace kiriview {
enum class DocumentSessionPublicSignal {
    PublicProjectionRevision,
    SourceUrl,
    DocumentKind,
    ErrorString,
    WindowTitleSubject,
    ActiveZoomReadout,
    ActiveMediaReadiness,
    DisplayedMediaOpenWithAvailability,
    DisplayedFileDeletionAvailability,
    FileDeletionInProgress,
    ActiveNavigation,
    ActiveNavigationRevealIntent,
    ActiveNavigationRevealDirection,
};

struct DocumentSessionPublicSignalOperations
{
    std::function<void()> publicProjectionRevisionChanged;
    std::function<void()> sourceUrlChanged;
    std::function<void()> documentKindChanged;
    std::function<void()> errorStringChanged;
    std::function<void()> windowTitleSubjectChanged;
    std::function<void()> activeZoomReadoutChanged;
    std::function<void()> activeMediaReadinessChanged;
    std::function<void()> displayedMediaOpenWithAvailabilityChanged;
    std::function<void()> displayedFileDeletionAvailabilityChanged;
    std::function<void()> fileDeletionInProgressChanged;
    std::function<void()> activeNavigationChanged;
    std::function<void()> activeNavigationRevealIntentChanged;
    std::function<void()> activeNavigationRevealDirectionChanged;
};

class DocumentSessionPublicSignalEmitter final
{
public:
    explicit DocumentSessionPublicSignalEmitter(DocumentSessionPublicSignalOperations operations);

    void emitChanges(const std::vector<DocumentSessionChange>& changes) const;
    void emitSignal(DocumentSessionPublicSignal signal) const;

private:
    DocumentSessionPublicSignalOperations m_operations;
};

std::vector<DocumentSessionPublicSignal> documentSessionPublicSignals(DocumentSessionChange change);
std::vector<DocumentSessionPublicSignal> documentSessionPublicSignalsForChanges(
    const std::vector<DocumentSessionChange>& changes);
}

#endif
