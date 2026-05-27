// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ACTIVENAVIGATIONBOUNDARYTEXT_H
#define KIRIVIEW_ACTIVENAVIGATIONBOUNDARYTEXT_H

#include "session/activenavigationprojection.h"

#include <QString>

namespace KiriView {
QString activeNavigationBoundaryFeedbackText(
    ActiveNavigationBoundaryScope scope, ActiveNavigationDispatchOutcome outcome);
}

#endif
