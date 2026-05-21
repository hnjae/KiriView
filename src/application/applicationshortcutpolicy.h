// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H
#define KIRIVIEW_APPLICATIONSHORTCUTPOLICY_H

#include "application/imageactionavailability.h"
#include "facade/kiriviewapplication.h"

#include <QKeySequence>
#include <QList>
#include <QString>
#include <QVariantList>

namespace KiriView::ApplicationActions {
enum class ApplicationShortcutFilter {
    AllShortcuts = 0,
    WithCommandModifier,
    WithoutCommandModifier,
    ShortcutAliases,
};

struct ApplicationShortcutRoute {
    QList<KiriViewApplication::ActionId> actionIds;
    ApplicationShortcutFilter shortcutFilter = ApplicationShortcutFilter::AllShortcuts;
    ImageActionAvailability::ShortcutScope shortcutScope
        = ImageActionAvailability::HelpShortcutScope;
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
ApplicationShortcutProjection shortcutProjection(const QList<QKeySequence> &shortcuts);
const QList<ApplicationShortcutRoute> &shortcutRoutes();
QVariantList shortcutRouteVariants();
}

#endif
