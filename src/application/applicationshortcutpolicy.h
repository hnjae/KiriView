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
    ApplicationShortcutFilter shortcutFilter = ApplicationShortcutFilter::AllShortcuts;
    ImageShortcutScope shortcutScope = ImageShortcutScope::HelpShortcutScope;
};

struct ApplicationShortcutProjection {
    QList<QKeySequence> shortcuts;
    QList<QKeySequence> shortcutsWithCommandModifier;
    QList<QKeySequence> shortcutsWithoutCommandModifier;
    QList<QKeySequence> shortcutAliases;
    QKeySequence menuShortcut;
    QString shortcutText;
    QString menuShortcutText;
};

QList<QKeySequence> filterShortcutsByCommandModifier(
    const QList<QKeySequence> &shortcuts, bool requireCommandModifier);
QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts);
QList<QKeySequence> shortcutAliases(const QList<QKeySequence> &shortcuts);
QString shortcutListText(const QList<QKeySequence> &shortcuts);
QList<QKeySequence> sanitizeShortcuts(const QList<QKeySequence> &shortcuts);
ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &shortcuts,
    ShortcutAliasPolicy aliasPolicy = ShortcutAliasPolicy::DeriveViewerAlias);
const QList<ApplicationShortcutRoute> &shortcutRoutes();
std::optional<ImageShortcutScope> imageShortcutScopeFromValue(int value);
bool videoActionUnsupported(ActionId actionId);
}

#endif
