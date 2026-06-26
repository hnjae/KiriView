// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONPROJECTION_H
#define KIRIVIEW_ACTIVENAVIGATIONPROJECTION_H

#include "navigation/directmedianavigationmodel.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <variant>

namespace kiriview {
enum class ActiveNavigationSourceKind {
    None,
    OrdinaryDirectMedia,
    ImageDocumentPages,
};

enum class ActiveNavigationBoundaryScope {
    None,
    DirectMedia,
    ImageDocumentPage,
};

enum class ActiveNavigationDispatchRequestKind {
    Previous,
    Next,
    First,
    Last,
    Number,
};

enum class ActiveNavigationDispatchOutcome {
    NoOp,
    Dispatch,
    FirstBoundary,
    LastBoundary,
};

struct OpenPreviousDirectMediaNavigationOperation
{
};
struct OpenNextDirectMediaNavigationOperation
{
};
struct OpenDirectMediaNavigationAtNumberOperation
{
    int number = 0;
};
struct OpenPreviousImageDocumentPageOperation
{
};
struct OpenNextImageDocumentPageOperation
{
};
struct OpenImageDocumentPageAtNumberOperation
{
    int number = 0;
};

using ActiveNavigationDispatchOperation = std::variant<std::monostate,
    OpenPreviousDirectMediaNavigationOperation, OpenNextDirectMediaNavigationOperation,
    OpenDirectMediaNavigationAtNumberOperation, OpenPreviousImageDocumentPageOperation,
    OpenNextImageDocumentPageOperation, OpenImageDocumentPageAtNumberOperation>;

struct ActiveNavigationSnapshot
{
    bool available = false;
    bool known = false;
    bool editable = false;
    bool canOpenPrevious = false;
    bool canOpenNext = false;
    bool atKnownFirst = false;
    bool atKnownLast = false;
    int currentNumber = 0;
    int count = 0;
};

struct DirectMediaActiveNavigationInput
{
    DirectMediaNavigationBoundaryState boundaryState;
    bool known = false;
};

struct ActiveNavigationDispatchRequest
{
    ActiveNavigationDispatchRequestKind kind = ActiveNavigationDispatchRequestKind::Next;
    int number = 0;
};

struct ActiveNavigationDispatchPlan
{
    ActiveNavigationDispatchOperation operation;
    ActiveNavigationDispatchOutcome outcome = ActiveNavigationDispatchOutcome::NoOp;

    bool shouldDispatch() const;
};

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    DirectMediaActiveNavigationInput directMediaInput,
    ImageDocumentPageActiveNavigationSnapshot imageDocumentPageSnapshot,
    bool fileDeletionInProgress);
ActiveNavigationSnapshot maskActiveNavigationDuringDeletion(ActiveNavigationSnapshot snapshot);
ActiveNavigationBoundaryScope activeNavigationBoundaryScopeForSource(
    ActiveNavigationSourceKind sourceKind);
ActiveNavigationDispatchRequest previousActiveNavigationDispatchRequest();
ActiveNavigationDispatchRequest nextActiveNavigationDispatchRequest();
ActiveNavigationDispatchRequest firstActiveNavigationDispatchRequest();
ActiveNavigationDispatchRequest lastActiveNavigationDispatchRequest();
ActiveNavigationDispatchRequest numberedActiveNavigationDispatchRequest(int number);
ActiveNavigationDispatchPlan activeNavigationDispatchPlan(ActiveNavigationSourceKind sourceKind,
    ActiveNavigationSnapshot snapshot, ActiveNavigationDispatchRequest request);
}

#endif
