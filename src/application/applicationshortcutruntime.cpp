// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutruntime.h"

#include "shortcuthelpmodel.h"
#include "shortcutroutemodel.h"

#include <KLocalizedString>
#include <KirigamiActionCollection>
#include <algorithm>
#include <utility>

namespace {
using ActionId = KiriView::ApplicationActions::ActionId;

struct ShortcutHelpCategory {
    QString key;
    QString text;
};

ShortcutHelpCategory shortcutHelpCategory(ActionId actionId)
{
    switch (actionId) {
    case ActionId::FileOpenAction:
    case ActionId::FileOpenWithAction:
    case ActionId::FileMoveToTrashAction:
    case ActionId::FileDeleteAction:
    case ActionId::FileQuitAction:
        return { QStringLiteral("file"), i18nc("@title:group", "File") };
    case ActionId::GoPreviousArchiveAction:
    case ActionId::GoNextArchiveAction:
    case ActionId::GoPreviousImageAction:
    case ActionId::GoNextImageAction:
    case ActionId::GoFirstImageAction:
    case ActionId::GoLastImageAction:
        return { QStringLiteral("navigation"), i18nc("@title:group", "Navigation") };
    case ActionId::ViewZoomInAction:
    case ActionId::ViewZoomOutAction:
    case ActionId::ViewFitAction:
    case ActionId::ViewFitHeightAction:
    case ActionId::ViewFitWidthAction:
    case ActionId::ViewActualSizeAction:
    case ActionId::ViewRotateClockwiseAction:
    case ActionId::ViewRotateCounterclockwiseAction:
    case ActionId::ViewToggleTwoPageModeAction:
    case ActionId::ViewToggleRightToLeftReadingAction:
    case ActionId::ViewPanTopLeftAction:
    case ActionId::ViewPanBottomRightAction:
    case ActionId::ViewScanForwardAction:
    case ActionId::ViewScanBackwardAction:
        return { QStringLiteral("view"), i18nc("@title:group", "View") };
    case ActionId::ViewToggleInfoPanelAction:
    case ActionId::ViewToggleThumbnailPanelAction:
        return { QStringLiteral("panels"), i18nc("@title:group", "Panels") };
    case ActionId::WindowFullscreenAction:
        return { QStringLiteral("window"), i18nc("@title:group", "Window") };
    case ActionId::OptionsConfigureKeybindingAction:
    case ActionId::OptionsShowMenubarAction:
        return { QStringLiteral("settings"), i18nc("@title:group", "Settings") };
    case ActionId::HelpShortcutsAction:
        return { QStringLiteral("help"), i18nc("@title:group", "Help") };
    case ActionId::OpenApplicationMenuAction:
    case ActionId::ActionCount:
        return { QStringLiteral("help"), i18nc("@title:group", "Help") };
    }

    return { QStringLiteral("help"), i18nc("@title:group", "Help") };
}

int shortcutHelpCategoryOrder(const QString &categoryKey)
{
    if (categoryKey == QStringLiteral("file")) {
        return 0;
    }
    if (categoryKey == QStringLiteral("navigation")) {
        return 1;
    }
    if (categoryKey == QStringLiteral("view")) {
        return 2;
    }
    if (categoryKey == QStringLiteral("panels")) {
        return 3;
    }
    if (categoryKey == QStringLiteral("window")) {
        return 4;
    }
    if (categoryKey == QStringLiteral("settings")) {
        return 5;
    }
    return 6;
}
}

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
    return shortcutKeyDisplayTexts(action).join(QStringLiteral(" / "));
}

QStringList ApplicationShortcutRuntime::shortcutKeyDisplayTexts(const QAction *action)
{
    QStringList texts;
    if (action != nullptr) {
        const QList<QKeySequence> shortcuts = action->shortcuts();
        texts.reserve(shortcuts.size());
        for (const QKeySequence &shortcut : shortcuts) {
            if (!shortcut.isEmpty()) {
                texts.push_back(shortcut.toString(QKeySequence::NativeText));
            }
        }
    }

    if (texts.isEmpty()) {
        texts.push_back(i18nc("@info:keyboard shortcut", "Unassigned"));
    }

    return texts;
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

        const ShortcutHelpCategory category = shortcutHelpCategory(registeredAction.actionId);
        rows.push_back(ShortcutHelpRow {
            static_cast<int>(registeredAction.actionId),
            registeredAction.actionName,
            actionDisplayText(registeredAction.action),
            shortcutDisplayText(registeredAction.action),
            category.key,
            category.text,
            shortcutKeyDisplayTexts(registeredAction.action),
        });
    }

    std::stable_sort(
        rows.begin(), rows.end(), [](const ShortcutHelpRow &left, const ShortcutHelpRow &right) {
            const int leftOrder = shortcutHelpCategoryOrder(left.categoryKey);
            const int rightOrder = shortcutHelpCategoryOrder(right.categoryKey);
            if (leftOrder != rightOrder) {
                return leftOrder < rightOrder;
            }
            return left.actionId < right.actionId;
        });

    for (qsizetype index = 0; index < rows.size(); ++index) {
        rows[index].categoryFirst
            = index == 0 || rows.at(index - 1).categoryKey != rows.at(index).categoryKey;
        rows[index].categoryLast = index == rows.size() - 1
            || rows.at(index + 1).categoryKey != rows.at(index).categoryKey;
    }

    return rows;
}
}
