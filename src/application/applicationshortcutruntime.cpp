// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutruntime.h"

#include "shortcuthelpmodel.h"
#include "shortcutroutemodel.h"

#include <KLocalizedString>
#include <KirigamiActionCollection>
#include <utility>

namespace KiriView::ApplicationActions {
ApplicationShortcutRuntime::ApplicationShortcutRuntime(ApplicationActionHost &host,
    const ApplicationActionRegistry &actionRegistry, ChangeCallback changeCallback)
    : m_host(host)
    , m_actionRegistry(actionRegistry)
    , m_changeCallback(std::move(changeCallback))
    , m_shortcutRouteModel(std::make_unique<ShortcutRouteModel>(m_host.actionContext()))
{
}

ApplicationShortcutRuntime::~ApplicationShortcutRuntime() = default;

void ApplicationShortcutRuntime::setup()
{
    sanitizeActionShortcuts();
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(
        [this]() { return shortcutHelpRows(); }, m_host.actionContext());
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
        m_shortcutHelpModel->handleRowsChanged();
    }
    ++m_shortcutRevision;
    if (m_changeCallback) {
        m_changeCallback();
    }
}

int ApplicationShortcutRuntime::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *ApplicationShortcutRuntime::shortcutHelpModel() const
{
    return m_shortcutHelpModel.get();
}

QAbstractListModel *ApplicationShortcutRuntime::shortcutRouteModel() const
{
    return m_shortcutRouteModel.get();
}

void ApplicationShortcutRuntime::sanitizeActionShortcuts()
{
    for (QAction *action : m_host.mainActionCollection()->actions()) {
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
    m_host.mainActionCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForAction(
    const QAction *action, ShortcutAliasPolicy aliasPolicy) const
{
    return ApplicationActions::shortcutProjection(
        action == nullptr ? QList<QKeySequence>() : action->shortcuts(), aliasPolicy);
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjection(
    const QString &actionName) const
{
    const ActionDefinition *definition = definitionForName(actionName);
    return shortcutProjectionForAction(m_actionRegistry.action(actionName),
        definition == nullptr ? ShortcutAliasPolicy::DeriveViewerAlias
                              : definition->shortcutAliasPolicy);
}

ApplicationShortcutProjection ApplicationShortcutRuntime::shortcutProjectionForId(
    ActionId actionId) const
{
    const ActionDefinition *definition = definitionForId(actionId);
    return shortcutProjectionForAction(m_actionRegistry.actionForId(actionId),
        definition == nullptr ? ShortcutAliasPolicy::DeriveViewerAlias
                              : definition->shortcutAliasPolicy);
}

QString ApplicationShortcutRuntime::actionDisplayText(const QAction *action)
{
    if (action == nullptr) {
        return {};
    }

    const QString text = action->text();
    QString displayText;
    displayText.reserve(text.size());
    for (qsizetype index = 0; index < text.size(); ++index) {
        if (text.at(index) != QLatin1Char('&')) {
            displayText.append(text.at(index));
            continue;
        }

        if (index + 1 < text.size() && text.at(index + 1) == QLatin1Char('&')) {
            displayText.append(QLatin1Char('&'));
            ++index;
        }
    }

    return displayText;
}

QString ApplicationShortcutRuntime::shortcutDisplayText(const QAction *action)
{
    if (action == nullptr) {
        return {};
    }

    const QString text = ApplicationActions::shortcutProjection(action->shortcuts()).shortcutText;
    return text.isEmpty() ? i18nc("@info:keyboard shortcut", "Unassigned") : text;
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

        rows.push_back(ShortcutHelpRow { static_cast<int>(registeredAction.actionId),
            registeredAction.actionName, actionDisplayText(registeredAction.action),
            shortcutDisplayText(registeredAction.action) });
    }

    return rows;
}
}
