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

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    MediaActiveNavigationInput mediaInput, ImageDocumentActiveNavigationInput imageInput,
    bool fileDeletionInProgress);
ActiveNavigationSnapshot maskActiveNavigationDuringDeletion(ActiveNavigationSnapshot snapshot);
ActiveNavigationBoundaryScope activeNavigationBoundaryScopeForSource(
    ActiveNavigationSourceKind sourceKind);
}

#endif
