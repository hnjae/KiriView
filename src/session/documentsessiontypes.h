// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONTYPES_H
#define KIRIVIEW_DOCUMENTSESSIONTYPES_H

#include "session/activenavigationprojection.h"

#include <QString>
#include <QtGlobal>

namespace KiriView {
enum class DocumentSessionKind {
    Empty,
    Image,
    Video,
};

enum class DocumentSessionChange {
    SourceUrl,
    DocumentKind,
    ErrorString,
    WindowTitleSubject,
    ActiveZoomReadout,
    OpenWithAvailability,
    FileDeletionAvailability,
    FileDeletionInProgress,
    ActiveNavigation,
    ActiveNavigationRevealIntent,
    ActiveNavigationRevealDirection,
};

enum class ActiveNavigationRevealIntent {
    None,
    ThumbnailActivation,
    AdjacentNavigation,
    LargeJump,
    LoadOrOpen,
    ProgrammaticSync,
};

enum class ActiveNavigationRevealDirection {
    None,
    Previous,
    Next,
};

struct ActiveNavigationRevealContext {
    ActiveNavigationRevealIntent intent = ActiveNavigationRevealIntent::None;
    ActiveNavigationRevealDirection direction = ActiveNavigationRevealDirection::None;
};

struct ActiveZoomSnapshot {
    bool available = false;
    bool known = false;
    qreal percent = 0.0;
    bool editable = false;
};

struct DocumentSessionPublicProjection {
    ActiveNavigationSourceKind sourceKind = ActiveNavigationSourceKind::None;
    ActiveNavigationBoundaryScope boundaryScope = ActiveNavigationBoundaryScope::None;
    ActiveNavigationSnapshot activeNavigation;
    QString windowTitleSubject;
    bool displayedMediaOpenWithAvailable = false;
    bool displayedFileDeletionAvailable = false;
};
}

#endif
