// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionruntime.h"

#include "applicationshortcutpolicy.h"
#include "kiriviewapplicationactions.h"
#include "kiriviewstate.h"
#include "shortcuthelpmodel.h"

#include <KirigamiActionCollection>
#include <QIcon>
#include <QSignalBlocker>

namespace {
namespace Actions = KiriView::ApplicationActions;

QString actionDefinitionName(const Actions::ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}

KiriViewApplication::MenuPresentation toMenuPresentation(int value)
{
    if (value == static_cast<int>(KiriViewApplication::MenuBar)) {
        return KiriViewApplication::MenuBar;
    }

    return KiriViewApplication::HamburgerMenu;
}
}

namespace KiriView::ApplicationActions {
ApplicationActionRuntime::ApplicationActionRuntime(KiriViewApplication &application)
    : m_application(application)
{
}

ApplicationActionRuntime::~ApplicationActionRuntime() = default;

KiriViewApplication::MenuPresentation ApplicationActionRuntime::menuPresentation() const
{
    return toMenuPresentation(KiriViewState::menuPresentation());
}

void ApplicationActionRuntime::setMenuPresentation(
    KiriViewApplication::MenuPresentation presentation)
{
    if (toMenuPresentation(KiriViewState::menuPresentation()) == presentation) {
        return;
    }

    KiriViewState::setMenuPresentation(static_cast<int>(presentation));
    KiriViewState::self()->save();
    updateShowMenuBarAction();
    Q_EMIT m_application.menuPresentationChanged();
}

int ApplicationActionRuntime::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *ApplicationActionRuntime::shortcutHelpModel() const
{
    return m_shortcutHelpModel.get();
}

QAction *ApplicationActionRuntime::action(const QString &actionName)
{
    return registeredAction(actionName);
}

QAction *ApplicationActionRuntime::actionForId(KiriViewApplication::ActionId actionId)
{
    return registeredActionForId(actionId);
}

QString ApplicationActionRuntime::actionName(KiriViewApplication::ActionId actionId) const
{
    return Actions::actionName(actionId);
}

QList<QKeySequence> ApplicationActionRuntime::shortcuts(const QString &actionName) const
{
    return shortcutProjection(actionName).shortcuts;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcuts;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return shortcutProjection(actionName).shortcutsWithCommandModifier;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutsWithCommandModifier;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return shortcutProjection(actionName).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithoutCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutAliases(const QString &actionName) const
{
    return shortcutProjection(actionName).shortcutAliases;
}

QList<QKeySequence> ApplicationActionRuntime::shortcutAliasesForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutAliases;
}

QString ApplicationActionRuntime::shortcutText(const QString &actionName) const
{
    return shortcutProjection(actionName).shortcutText;
}

QString ApplicationActionRuntime::shortcutTextForId(KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).shortcutText;
}

QKeySequence ApplicationActionRuntime::menuShortcut(const QString &actionName) const
{
    return shortcutProjection(actionName).menuShortcut;
}

QKeySequence ApplicationActionRuntime::menuShortcutForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).menuShortcut;
}

QString ApplicationActionRuntime::menuShortcutText(const QString &actionName) const
{
    return shortcutProjection(actionName).menuShortcutText;
}

QString ApplicationActionRuntime::menuShortcutTextForId(
    KiriViewApplication::ActionId actionId) const
{
    return shortcutProjectionForId(actionId).menuShortcutText;
}

void ApplicationActionRuntime::setupActions()
{
    m_application.mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const Actions::ActionDefinition &definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts
            = Actions::defaultShortcuts(definition.defaultShortcuts);

        switch (definition.kind) {
        case Actions::RegistrationKind::Existing:
            if (QAction *action = registeredAction(name)) {
                finishRegisteredAction(action, action->text(), shortcuts);
            }
            return;
        case Actions::RegistrationKind::Inherited:
            return;
        case Actions::RegistrationKind::Registered:
            addRegisteredAction(name, Actions::localizedString(definition.text),
                Actions::latin1String(definition.iconName), shortcuts);
            return;
        case Actions::RegistrationKind::ShowMenubar:
            m_showMenuBarAction = addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            KirigamiActionCollection::setShortcutsConfigurable(m_showMenuBarAction, false);
            m_showMenuBarAction->setCheckable(true);
            QObject::connect(
                m_showMenuBarAction, &QAction::triggered, &m_application, [this](bool checked) {
                    setMenuPresentation(checked ? KiriViewApplication::MenuBar
                                                : KiriViewApplication::HamburgerMenu);
                });
            return;
        case Actions::RegistrationKind::Standard:
            addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            return;
        }
    };

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        addAction(definition);
    }

    m_application.readSettings();
    sanitizeActionShortcuts();
    updateShowMenuBarAction();
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(
        [this]() { return shortcutHelpRows(); }, &m_application);
}

QAction *ApplicationActionRuntime::addRegisteredAction(const QString &name, const QString &text,
    const QString &iconName, const QList<QKeySequence> &defaultShortcuts)
{
    auto *action = new QAction(&m_application);
    action->setObjectName(name);
    action->setText(text);
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }

    m_application.mainCollection()->addAction(name, action);
    KirigamiActionCollection::setDefaultShortcuts(action, defaultShortcuts);
    QObject::connect(action, &QAction::changed, &m_application,
        [this, action]() { handleActionChanged(action); });
    return action;
}

QAction *ApplicationActionRuntime::addStandardAction(KStandardActions::StandardAction actionType,
    const QString &name, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    QAction *action
        = m_application.mainCollection()->addAction(actionType, name, &m_application, [](bool) { });
    return finishRegisteredAction(action, text, defaultShortcuts);
}

QAction *ApplicationActionRuntime::finishRegisteredAction(
    QAction *action, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    action->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(action, defaultShortcuts);
    QObject::connect(action, &QAction::changed, &m_application,
        [this, action]() { handleActionChanged(action); });
    return action;
}

void ApplicationActionRuntime::handleActionChanged(QAction *changedAction)
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

void ApplicationActionRuntime::sanitizeActionShortcuts()
{
    for (QAction *action : m_application.mainCollection()->actions()) {
        sanitizeActionShortcuts(action);
    }
}

void ApplicationActionRuntime::sanitizeActionShortcuts(QAction *action)
{
    if (action == nullptr) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = Actions::sanitizeShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    m_application.mainCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

void ApplicationActionRuntime::updateShowMenuBarAction()
{
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == KiriViewApplication::MenuBar);
}

QAction *ApplicationActionRuntime::registeredAction(const QString &actionName) const
{
    return const_cast<KiriViewApplication &>(m_application)
        .AbstractKirigamiApplication::action(actionName);
}

QAction *ApplicationActionRuntime::registeredActionForId(
    KiriViewApplication::ActionId actionId) const
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return nullptr;
    }

    return registeredAction(name);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjection(
    const QString &actionName) const
{
    const QAction *registered = registeredAction(actionName);
    return Actions::shortcutProjection(
        registered == nullptr ? QList<QKeySequence>() : registered->shortcuts());
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjectionForId(
    KiriViewApplication::ActionId actionId) const
{
    const QAction *registered = registeredActionForId(actionId);
    return Actions::shortcutProjection(
        registered == nullptr ? QList<QKeySequence>() : registered->shortcuts());
}

QList<ShortcutHelpRow> ApplicationActionRuntime::shortcutHelpRows() const
{
    QList<ShortcutHelpRow> rows;
    rows.reserve(static_cast<qsizetype>(Actions::definitions().size()));

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString name = actionDefinitionName(definition);
        QAction *action = registeredAction(name);
        if (action == nullptr || !KirigamiActionCollection::isShortcutsConfigurable(action)) {
            continue;
        }

        rows.push_back(ShortcutHelpRow { action, static_cast<int>(definition.actionId), name });
    }

    return rows;
}
}
