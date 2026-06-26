// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_WINDOWTITLEPROJECTION_H
#define KIRIVIEW_WINDOWTITLEPROJECTION_H

#include "session/activenavigationprojection.h"

#include <QSize>
#include <QString>

namespace kiriview {
struct WindowTitleSubjectInput
{
    QString baseName;
    ActiveNavigationSourceKind sourceKind = ActiveNavigationSourceKind::None;
    QSize directMediaSize;
    ActiveNavigationSnapshot activeNavigation;
};

QString projectWindowTitleSubject(WindowTitleSubjectInput input);
}

#endif
