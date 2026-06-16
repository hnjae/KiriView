// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONACTIVENAVIGATIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONACTIVENAVIGATIONRUNTIME_H

#include "session/activenavigationprojection.h"
#include "session/documentsessiontypes.h"

#include <functional>

namespace kiriview {
struct DocumentSessionActiveNavigationRuntimePorts {
    std::function<void(ActiveNavigationRevealContext)> setRevealContext;
    std::function<void()> recomputePublicProjection;
    std::function<void()> openPreviousDirectMedia;
    std::function<void()> openNextDirectMedia;
    std::function<void(int)> openDirectMediaAtNumber;
    std::function<void()> openPreviousImageDocumentPage;
    std::function<void()> openNextImageDocumentPage;
    std::function<void(int)> openImageDocumentPageAtNumber;
};

class DocumentSessionActiveNavigationRuntime final
{
public:
    explicit DocumentSessionActiveNavigationRuntime(
        DocumentSessionActiveNavigationRuntimePorts ports = {});

    ActiveNavigationDispatchOutcome dispatch(ActiveNavigationSourceKind sourceKind,
        ActiveNavigationSnapshot snapshot, ActiveNavigationDispatchRequest request,
        ActiveNavigationRevealContext context);
    void setPendingRevealContext(ActiveNavigationRevealContext context);
    ActiveNavigationRevealContext takePendingRevealContext(
        ActiveNavigationRevealIntent fallbackIntent);
    void setRevealContext(ActiveNavigationRevealContext context);
    void clearRevealContextIfUnavailable(const ActiveNavigationSnapshot &snapshot);

private:
    void executeDispatchPlan(const ActiveNavigationDispatchPlan &plan);

    DocumentSessionActiveNavigationRuntimePorts m_ports;
    ActiveNavigationRevealContext m_pendingRevealContext;
};
}

#endif
