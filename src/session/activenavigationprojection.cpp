// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationprojection.h"

#include "bridge/activenavigationprojectionconversion.h"
#include "kiriview/src/policy/activenavigation.cxx.h"

#include <variant>

namespace KiriView {
bool ActiveNavigationDispatchPlan::shouldDispatch() const
{
    return outcome == ActiveNavigationDispatchOutcome::Dispatch
        && !std::holds_alternative<std::monostate>(operation);
}

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    DirectMediaActiveNavigationInput directMediaInput,
    ImageDocumentPageActiveNavigationSnapshot imageDocumentPageSnapshot,
    bool fileDeletionInProgress)
{
    return Bridge::activeNavigationSnapshotFromRust(
        rustProjectActiveNavigation(Bridge::rustActiveNavigationSourceKind(sourceKind),
            Bridge::rustDirectMediaActiveNavigationInput(directMediaInput),
            Bridge::rustImageDocumentPageActiveNavigationSnapshot(imageDocumentPageSnapshot),
            fileDeletionInProgress));
}

ActiveNavigationSnapshot maskActiveNavigationDuringDeletion(ActiveNavigationSnapshot snapshot)
{
    return Bridge::activeNavigationSnapshotFromRust(
        rustMaskActiveNavigationDuringDeletion(Bridge::rustActiveNavigationSnapshot(snapshot)));
}

ActiveNavigationBoundaryScope activeNavigationBoundaryScopeForSource(
    ActiveNavigationSourceKind sourceKind)
{
    return Bridge::activeNavigationBoundaryScopeFromRust(rustActiveNavigationBoundaryScopeForSource(
        Bridge::rustActiveNavigationSourceKind(sourceKind)));
}

ActiveNavigationDispatchRequest previousActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Previous, 0 };
}

ActiveNavigationDispatchRequest nextActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Next, 0 };
}

ActiveNavigationDispatchRequest firstActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::First, 1 };
}

ActiveNavigationDispatchRequest lastActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Last, 0 };
}

ActiveNavigationDispatchRequest numberedActiveNavigationDispatchRequest(int number)
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Number, number };
}

ActiveNavigationDispatchPlan activeNavigationDispatchPlan(ActiveNavigationSourceKind sourceKind,
    ActiveNavigationSnapshot snapshot, ActiveNavigationDispatchRequest request)
{
    return Bridge::activeNavigationDispatchPlanFromRust(
        rustActiveNavigationDispatchPlan(Bridge::rustActiveNavigationSourceKind(sourceKind),
            Bridge::rustActiveNavigationSnapshot(snapshot),
            Bridge::rustActiveNavigationDispatchRequest(request)));
}
}
