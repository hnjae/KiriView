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
    const QAction *action) const
{
    return ApplicationActions::shortcutProjection(
        action == nullptr ? QList<QKeySequence>() : action->shortcuts());
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjection(
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
