// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONPROJECTION_H
#define KIRIVIEW_ACTIVENAVIGATIONPROJECTION_H

#include "navigation/medianavigationmodel.h"

namespace KiriView {
enum class ActiveNavigationSourceKind {
    None,
    OrdinaryDirectMedia,
    ImageDocumentPages,
};

enum class ActiveNavigationBoundaryScope {
    None,
    Media,
    ImageDocument,
};

enum class ActiveNavigationDispatchRequestKind {
    Previous,
    Next,
    First,
    Last,
    Number,
};

enum class ActiveNavigationDispatchTarget {
    None,
    OrdinaryDirectMedia,
    ImageDocumentPages,
};

enum class ActiveNavigationDispatchOperation {
    None,
    OpenPrevious,
    OpenNext,
    OpenNumber,
};

enum class ActiveNavigationDispatchOutcome {
    NoOp,
    Dispatch,
    FirstBoundary,
    LastBoundary,
};

struct ActiveNavigationSnapshot {
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

struct MediaActiveNavigationInput {
    MediaNavigationBoundaryState boundaryState;
    bool known = false;
};

struct ImageDocumentActiveNavigationInput {
    int currentNumber = 0;
    int currentLastNumber = 0;
    int count = 0;
};

struct ActiveNavigationDispatchRequest {
    ActiveNavigationDispatchRequestKind kind = ActiveNavigationDispatchRequestKind::Next;
    int number = 0;
};

struct ActiveNavigationDispatchPlan {
    ActiveNavigationDispatchTarget target = ActiveNavigationDispatchTarget::None;
    ActiveNavigationDispatchOperation operation = ActiveNavigationDispatchOperation::None;
    ActiveNavigationDispatchOutcome outcome = ActiveNavigationDispatchOutcome::NoOp;
    int number = 0;

    bool shouldDispatch() const;
};

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    MediaActiveNavigationInput mediaInput, ImageDocumentActiveNavigationInput imageInput,
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
