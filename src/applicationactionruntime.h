// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONRUNTIME_H
#define KIRIVIEW_APPLICATIONACTIONRUNTIME_H

#include "applicationshortcutpolicy.h"
#include "kiriviewapplication.h"

#include <KStandardActions>
#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <memory>

namespace KiriView::ApplicationActions {
class ShortcutHelpModel;
struct ShortcutHelpRow;

class ApplicationActionRuntime final
{
public:
    explicit ApplicationActionRuntime(KiriViewApplication &application);
    ~ApplicationActionRuntime();

    KiriViewApplication::MenuPresentation menuPresentation() const;
    void setMenuPresentation(KiriViewApplication::MenuPresentation presentation);
    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;

    QAction *action(const QString &actionName);
    QAction *actionForId(KiriViewApplication::ActionId actionId);
    QString actionName(KiriViewApplication::ActionId actionId) const;
    QList<QKeySequence> shortcuts(const QString &actionName) const;
    QList<QKeySequence> shortcutsForId(KiriViewApplication::ActionId actionId) const;
    QList<QKeySequence> shortcutsWithCommandModifier(const QString &actionName) const;
    QList<QKeySequence> shortcutsWithCommandModifierForId(
        KiriViewApplication::ActionId actionId) const;
    QList<QKeySequence> shortcutsWithoutCommandModifier(const QString &actionName) const;
    QList<QKeySequence> shortcutsWithoutCommandModifierForId(
        KiriViewApplication::ActionId actionId) const;
    QList<QKeySequence> shortcutAliases(const QString &actionName) const;
    QList<QKeySequence> shortcutAliasesForId(KiriViewApplication::ActionId actionId) const;
    QString shortcutText(const QString &actionName) const;
    QString shortcutTextForId(KiriViewApplication::ActionId actionId) const;
    QKeySequence menuShortcut(const QString &actionName) const;
    QKeySequence menuShortcutForId(KiriViewApplication::ActionId actionId) const;
    QString menuShortcutText(const QString &actionName) const;
    QString menuShortcutTextForId(KiriViewApplication::ActionId actionId) const;

    void setupActions();

private:
    QAction *addRegisteredAction(const QString &name, const QString &text, const QString &iconName,
        const QList<QKeySequence> &defaultShortcuts = {});
    QAction *addStandardAction(KStandardActions::StandardAction actionType, const QString &name,
        const QString &text, const QList<QKeySequence> &defaultShortcuts);
    QAction *finishRegisteredAction(QAction *registeredAction, const QString &text,
        const QList<QKeySequence> &defaultShortcuts);
    void handleActionChanged(QAction *changedAction);
    void sanitizeActionShortcuts();
    void sanitizeActionShortcuts(QAction *action);
    void updateShowMenuBarAction();
    QAction *registeredAction(const QString &actionName) const;
    QAction *registeredActionForId(KiriViewApplication::ActionId actionId) const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(
        KiriViewApplication::ActionId actionId) const;
    QList<ShortcutHelpRow> shortcutHelpRows() const;

    KiriViewApplication &m_application;
    QAction *m_showMenuBarAction = nullptr;
    std::unique_ptr<ShortcutHelpModel> m_shortcutHelpModel;
    int m_shortcutRevision = 0;
    bool m_sanitizingShortcuts = false;
};
}

#endif
