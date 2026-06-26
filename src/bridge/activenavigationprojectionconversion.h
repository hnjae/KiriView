// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_ACTIVENAVIGATIONPROJECTIONCONVERSION_H
#define KIRIVIEW_BRIDGE_ACTIVENAVIGATIONPROJECTIONCONVERSION_H

#include "kiriview/src/policy/activenavigation.cxx.h"
#include "session/activenavigationprojection.h"

namespace kiriview::Bridge {
RustActiveNavigationSourceKind rustActiveNavigationSourceKind(
    ActiveNavigationSourceKind sourceKind);
RustDirectMediaNavigationBoundaryState rustDirectMediaNavigationBoundaryState(
    DirectMediaNavigationBoundaryState state);
RustDirectMediaActiveNavigationInput rustDirectMediaActiveNavigationInput(
    DirectMediaActiveNavigationInput input);
RustImageDocumentPageActiveNavigationSnapshot rustImageDocumentPageActiveNavigationSnapshot(
    ImageDocumentPageActiveNavigationSnapshot snapshot);
RustActiveNavigationSnapshot rustActiveNavigationSnapshot(ActiveNavigationSnapshot snapshot);
RustActiveNavigationDispatchRequest rustActiveNavigationDispatchRequest(
    ActiveNavigationDispatchRequest request);
ActiveNavigationBoundaryScope activeNavigationBoundaryScopeFromRust(
    RustActiveNavigationBoundaryScope scope);
ActiveNavigationSnapshot activeNavigationSnapshotFromRust(RustActiveNavigationSnapshot snapshot);
ActiveNavigationDispatchPlan activeNavigationDispatchPlanFromRust(
    RustActiveNavigationDispatchPlan plan);
}

#endif
