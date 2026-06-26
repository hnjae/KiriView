// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPROJECTIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONPROJECTIONRUNTIME_H

#include "navigation/directmedianavigationmodel.h"
#include "session/activenavigationprojection.h"
#include "session/activenavigationthumbnailprojection.h"
#include "session/documentsessionpublicprojection.h"

#include <functional>
#include <vector>

namespace kiriview {
struct DocumentSessionProjectionRuntimePorts
{
    std::function<bool(const DocumentSessionPublicSnapshotInput&)> updatePublicSnapshot;
    std::function<bool(const DocumentSessionPublicSnapshotInput&, ActiveNavigationSourceKind)>
        updatePublicSnapshotForSourceKind;
    std::function<ActiveNavigationSourceKind()> activeNavigationSourceKind;
    std::function<ActiveNavigationSnapshot()> activeNavigationSnapshot;
    std::function<std::vector<DirectMediaNavigationCandidate>()> directMediaNavigationCandidates;
    std::function<void(std::vector<ActiveNavigationThumbnailRow>)> setActiveNavigationThumbnailRows;
    std::function<void()> clearActiveNavigationRevealContextIfUnavailable;
};

class DocumentSessionProjectionRuntime final
{
public:
    explicit DocumentSessionProjectionRuntime(DocumentSessionProjectionRuntimePorts ports = {});

    void publish(const DocumentSessionPublicSnapshotInput& input,
        const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows);
    void publishForSourceKind(const DocumentSessionPublicSnapshotInput& input,
        ActiveNavigationSourceKind sourceKind,
        const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows);

private:
    void syncActiveNavigationThumbnailRows(
        const ImageDocumentPageNavigationSnapshot& imageDocumentPageNavigationRows);
    void clearActiveNavigationRevealContextIfUnavailable();

    DocumentSessionProjectionRuntimePorts m_ports;
};
}

#endif
