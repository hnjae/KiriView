// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H
#define KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H

#include "application/applicationtypes.h"
#include "application/imageactionavailabilitypolicy.h"
#include "application/kiriviewapplicationactions.h"

#include <QKeySequence>
#include <QList>
#include <QString>
#include <optional>

namespace KiriView::ApplicationActions {
struct ApplicationShortcutRoute {
    QList<ActionId> actionIds;
    ApplicationShortcutActivationScope activationScope
        = ApplicationShortcutActivationScope::ProgramWide;
    ImageShortcutScope shortcutScope = ImageShortcutScope::HelpShortcutScope;
};

struct ApplicationShortcutProjection {
    QList<QKeySequence> shortcuts;
    QList<QKeySequence> programWideShortcuts;
    QList<QKeySequence> viewerLocalShortcuts;
    QKeySequence menuShortcut;
    QString shortcutText;
    QString menuShortcutText;
};

QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts);
QString shortcutListText(const QList<QKeySequence> &shortcuts);
QList<QKeySequence> sanitizeProgramWideShortcuts(const QList<QKeySequence> &shortcuts);
ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &programWideShortcuts,
    const QList<QKeySequence> &viewerLocalShortcuts = {});
const QList<ApplicationShortcutRoute> &shortcutRoutes();
std::optional<ImageShortcutScope> imageShortcutScopeFromValue(int value);
bool videoActionUnsupported(ActionId actionId);
}

#endif
