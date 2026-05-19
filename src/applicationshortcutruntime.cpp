// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutruntime.h"

#include "shortcuthelpmodel.h"

#include <KirigamiActionCollection>

namespace KiriView::ApplicationActions {
ApplicationShortcutRuntime::ApplicationShortcutRuntime(
    KiriViewApplication &application, const ApplicationActionRegistry &actionRegistry)
    : m_application(application)
    , m_actionRegistry(actionRegistry)
{
}

ApplicationShortcutRuntime::~ApplicationShortcutRuntime() = default;

void ApplicationShortcutRuntime::setup()
{
    sanitizeActionShortcuts();
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(
        [this]() { return shortcutHelpRows(); }, &m_application);
}

void ApplicationShortcutRuntime::handleActionChanged(QAction *changedAction)
{
    if (m_sanitizingShortcuts) {
        return;
    }

    if (changedAction != nullptr) {
        sanitizeActionShortcuts(changedAction);
    }

    if (m_shortcutHelpModel != nullptr) {
        m_shortcutHelpModel->handleActionChanged(changedAction);
    }
    ++m_shortcutRevision;
    Q_EMIT m_application.shortcutRevisionChanged();
}

int ApplicationShortcutRuntime::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *ApplicationShortcutRuntime::shortcutHelpModel() const
{
    return m_shortcutHelpModel.get();
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcuts(const QString &actionName) const
{
    return shortcutProjectionForName(actionName).shortcuts;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutsForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcuts;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return shortcutProjectionForName(actionName).shortcutsWithCommandModifier;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutsWithCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutsWithCommandModifier;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return shortcutProjectionForName(actionName).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutsWithoutCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutAliases(const QString &actionName) const
{
    return shortcutProjectionForName(actionName).shortcutAliases;
}

QList<QKeySequence> ApplicationShortcutRuntime::shortcutAliasesForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutAliases;
}

QString ApplicationShortcutRuntime::shortcutText(const QString &actionName) const
{
    return shortcutProjectionForName(actionName).shortcutText;
}

QString ApplicationShortcutRuntime::shortcutTextForId(KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutText;
}

QKeySequence ApplicationShortcutRuntime::menuShortcut(const QString &actionName) const
{
    return shortcutProjectionForName(actionName).menuShortcut;
}

QKeySequence ApplicationShortcutRuntime::menuShortcutForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).menuShortcut;
}

QString ApplicationShortcutRuntime::menuShortcutText(const QString &actionName) const
{
    return shortcutProjectionForName(actionName).menuShortcutText;
}

QString ApplicationShortcutRuntime::menuShortcutTextForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).menuShortcutText;
}

void ApplicationShortcutRuntime::sanitizeActionShortcuts()
{
    for (QAction *action : m_application.mainCollection()->actions()) {
        sanitizeActionShortcuts(action);
    }
}

void ApplicationShortcutRuntime::sanitizeActionShortcuts(QAction *action)
{
    if (action == nullptr) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = sanitizeShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    m_application.mainCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForAction(
    const QAction *registeredAction) const
{
    return shortcutProjection(
        registeredAction == nullptr ? QList<QKeySequence>() : registeredAction->shortcuts());
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForName(
    const QString &actionName) const
{
    return shortcutProjectionForAction(m_actionRegistry.action(actionName));
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForAction(m_actionRegistry.actionForId(actionId));
}

QList<ShortcutHelpRow> ApplicationShortcutRuntime::shortcutHelpRows() const
{
    QList<ShortcutHelpRow> rows;
    rows.reserve(static_cast<qsizetype>(definitions().size()));

    for (const RegisteredApplicationAction &registeredAction :
        m_actionRegistry.registeredActions()) {
        if (!KirigamiActionCollection::isShortcutsConfigurable(registeredAction.action)) {
            continue;
        }

        rows.push_back(ShortcutHelpRow { registeredAction.action,
            static_cast<int>(registeredAction.actionId), registeredAction.actionName });
    }

    return rows;
}
}
