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
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return nullptr;
    }

    return action(name);
}

QString ApplicationActionRuntime::actionName(KiriViewApplication::ActionId actionId) const
{
    return Actions::actionName(actionId);
}

QList<QKeySequence> ApplicationActionRuntime::shortcuts(const QString &actionName) const
{
    const QAction *action = registeredAction(actionName);
    if (action == nullptr) {
        return {};
    }

    return action->shortcuts();
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsForId(
    KiriViewApplication::ActionId actionId) const
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return {};
    }

    return shortcuts(name);
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), true);
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), true);
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), false);
}

QList<QKeySequence> ApplicationActionRuntime::shortcutsWithoutCommandModifierForId(
    KiriViewApplication::ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), false);
}

QList<QKeySequence> ApplicationActionRuntime::shortcutAliases(const QString &actionName) const
{
    return Actions::shortcutAliases(shortcuts(actionName));
}

QList<QKeySequence> ApplicationActionRuntime::shortcutAliasesForId(
    KiriViewApplication::ActionId actionId) const
{
    return Actions::shortcutAliases(shortcutsForId(actionId));
}

QString ApplicationActionRuntime::shortcutText(const QString &actionName) const
{
    return Actions::shortcutListText(shortcuts(actionName));
}

QString ApplicationActionRuntime::shortcutTextForId(KiriViewApplication::ActionId actionId) const
{
    return Actions::shortcutListText(shortcutsForId(actionId));
}

QKeySequence ApplicationActionRuntime::menuShortcut(const QString &actionName) const
{
    return Actions::menuShortcut(shortcuts(actionName));
}

QKeySequence ApplicationActionRuntime::menuShortcutForId(
    KiriViewApplication::ActionId actionId) const
{
    return Actions::menuShortcut(shortcutsForId(actionId));
}

QString ApplicationActionRuntime::menuShortcutText(const QString &actionName) const
{
    return menuShortcut(actionName).toString(QKeySequence::NativeText);
}

QString ApplicationActionRuntime::menuShortcutTextForId(
    KiriViewApplication::ActionId actionId) const
{
    return menuShortcutForId(actionId).toString(QKeySequence::NativeText);
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
    m_shortcutHelpModel = std::make_unique<ShortcutHelpModel>(m_application, &m_application);
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
}
