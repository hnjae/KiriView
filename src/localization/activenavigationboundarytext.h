// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONBOUNDARYTEXT_H
#define KIRIVIEW_ACTIVENAVIGATIONBOUNDARYTEXT_H

#include "navigation/imagedocumentpagenavigationtypes.h"
#include "session/activenavigationprojection.h"

#include <QString>

namespace kiriview {
QString activeNavigationBoundaryFeedbackText(
    ActiveNavigationBoundaryScope scope, ActiveNavigationDispatchOutcome outcome);
QString containerNavigationBoundaryFeedbackText(NavigationDirection direction);
}

#endif
