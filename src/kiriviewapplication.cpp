// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"

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

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
{
    KiriViewApplication::setupActions();
    m_shortcutHelpModel = new Actions::ShortcutHelpModel(*this, this);
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return toMenuPresentation(KiriViewState::menuPresentation());
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    if (toMenuPresentation(KiriViewState::menuPresentation()) == presentation) {
        return;
    }

    KiriViewState::setMenuPresentation(static_cast<int>(presentation));
    KiriViewState::self()->save();
    updateShowMenuBarAction();
    Q_EMIT menuPresentationChanged();
}

int KiriViewApplication::shortcutRevision() const { return m_shortcutRevision; }

QAbstractListModel *KiriViewApplication::shortcutHelpModel() const { return m_shortcutHelpModel; }

QAction *KiriViewApplication::action(const QString &actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return nullptr;
    }

    return action(name);
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return Actions::actionName(actionId);
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    const QAction *registeredAction = const_cast<KiriViewApplication *>(this)->action(actionName);
    if (!registeredAction) {
        return {};
    }

    return registeredAction->shortcuts();
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    const QString name = actionName(actionId);
    if (name.isEmpty()) {
        return {};
    }

    return shortcuts(name);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), true);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return Actions::filterShortcutsByCommandModifier(shortcuts(actionName), false);
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return Actions::filterShortcutsByCommandModifier(shortcutsForId(actionId), false);
}

QList<QKeySequence> KiriViewApplication::shortcutAliases(const QString &actionName) const
{
    return Actions::shortcutAliases(shortcuts(actionName));
}

QList<QKeySequence> KiriViewApplication::shortcutAliasesForId(ActionId actionId) const
{
    return Actions::shortcutAliases(shortcutsForId(actionId));
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return Actions::shortcutListText(shortcuts(actionName));
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return Actions::shortcutListText(shortcutsForId(actionId));
}

QKeySequence KiriViewApplication::menuShortcut(const QString &actionName) const
{
    return Actions::menuShortcut(shortcuts(actionName));
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return Actions::menuShortcut(shortcutsForId(actionId));
}

QString KiriViewApplication::menuShortcutText(const QString &actionName) const
{
    return menuShortcut(actionName).toString(QKeySequence::NativeText);
}

QString KiriViewApplication::menuShortcutTextForId(ActionId actionId) const
{
    return menuShortcutForId(actionId).toString(QKeySequence::NativeText);
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const Actions::ActionDefinition &definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts
            = Actions::defaultShortcuts(definition.defaultShortcuts);

        switch (definition.kind) {
        case Actions::RegistrationKind::Existing:
            if (QAction *registeredAction = action(name)) {
                finishRegisteredAction(registeredAction, registeredAction->text(), shortcuts);
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
            connect(m_showMenuBarAction, &QAction::triggered, this,
                [this](bool checked) { setMenuPresentation(checked ? MenuBar : HamburgerMenu); });
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

    readSettings();
    sanitizeActionShortcuts();
    updateShowMenuBarAction();
}

QAction *KiriViewApplication::addRegisteredAction(const QString &name, const QString &text,
    const QString &iconName, const QList<QKeySequence> &defaultShortcuts)
{
    auto *registeredAction = new QAction(this);
    registeredAction->setObjectName(name);
    registeredAction->setText(text);
    if (!iconName.isEmpty()) {
        registeredAction->setIcon(QIcon::fromTheme(iconName));
    }

    mainCollection()->addAction(name, registeredAction);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

QAction *KiriViewApplication::addStandardAction(KStandardActions::StandardAction actionType,
    const QString &name, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    QAction *registeredAction = mainCollection()->addAction(actionType, name, this, [](bool) { });
    return finishRegisteredAction(registeredAction, text, defaultShortcuts);
}

QAction *KiriViewApplication::finishRegisteredAction(
    QAction *registeredAction, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    registeredAction->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(registeredAction, defaultShortcuts);
    connect(registeredAction, &QAction::changed, this, &KiriViewApplication::handleActionChanged);
    return registeredAction;
}

void KiriViewApplication::handleActionChanged()
{
    if (m_sanitizingShortcuts) {
        return;
    }

    auto *changedAction = qobject_cast<QAction *>(sender());
    if (changedAction != nullptr) {
        sanitizeActionShortcuts(changedAction);
    }

    if (m_shortcutHelpModel != nullptr) {
        m_shortcutHelpModel->handleActionChanged(changedAction);
    }
    ++m_shortcutRevision;
    Q_EMIT shortcutRevisionChanged();
}

void KiriViewApplication::sanitizeActionShortcuts()
{
    for (QAction *registeredAction : mainCollection()->actions()) {
        sanitizeActionShortcuts(registeredAction);
    }
}

void KiriViewApplication::sanitizeActionShortcuts(QAction *action)
{
    if (!action) {
        return;
    }

    const QList<QKeySequence> shortcuts = action->shortcuts();
    const QList<QKeySequence> sanitizedShortcuts = Actions::sanitizeShortcuts(shortcuts);
    if (sanitizedShortcuts == shortcuts) {
        return;
    }

    m_sanitizingShortcuts = true;
    action->setShortcuts(sanitizedShortcuts);
    mainCollection()->writeSettings(nullptr, false, action);
    m_sanitizingShortcuts = false;
}

void KiriViewApplication::updateShowMenuBarAction()
{
    if (!m_showMenuBarAction) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == MenuBar);
}
