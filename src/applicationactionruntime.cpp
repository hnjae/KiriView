// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionruntime.h"

#include "applicationshortcutruntime.h"
#include "kiriviewapplicationactions.h"
#include "kiriviewstate.h"

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
    , m_actionRegistry(application)
    , m_shortcutRuntime(
          std::make_unique<ApplicationShortcutRuntime>(m_application, m_actionRegistry))
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

int ApplicationActionRuntime::shortcutRevision() const
{
    return m_shortcutRuntime->shortcutRevision();
}

QAbstractListModel *ApplicationActionRuntime::shortcutHelpModel() const
{
    return m_shortcutRuntime->shortcutHelpModel();
}

QAction *ApplicationActionRuntime::action(const QString &actionName)
{
    return m_actionRegistry.action(actionName);
}

QAction *ApplicationActionRuntime::actionForId(KiriViewApplication::ActionId actionId)
{
    return m_actionRegistry.actionForId(actionId);
}

QString ApplicationActionRuntime::actionName(KiriViewApplication::ActionId actionId) const
{
    return m_actionRegistry.actionName(actionId);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjection(
    const QString &actionName) const
{
    return m_shortcutRuntime->shortcutProjection(actionName);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjectionForId(
    KiriViewApplication::ActionId actionId) const
{
    return m_shortcutRuntime->shortcutProjectionForId(actionId);
}

void ApplicationActionRuntime::setupActions()
{
    m_application.mainCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const Actions::ActionDefinition &definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts
            = Actions::defaultShortcuts(definition.defaultShortcuts);
        QAction *registeredAction = nullptr;

        switch (definition.kind) {
        case Actions::RegistrationKind::Existing:
            if (QAction *action = m_actionRegistry.collectionAction(name)) {
                registeredAction = finishRegisteredAction(action, action->text(), shortcuts);
            }
            break;
        case Actions::RegistrationKind::Inherited:
            registeredAction = m_actionRegistry.collectionAction(name);
            break;
        case Actions::RegistrationKind::Registered:
            registeredAction = addRegisteredAction(name, Actions::localizedString(definition.text),
                Actions::latin1String(definition.iconName), shortcuts);
            break;
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
            registeredAction = m_showMenuBarAction;
            break;
        case Actions::RegistrationKind::Standard:
            registeredAction = addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            break;
        }

        m_actionRegistry.registerAction(definition, registeredAction);
    };

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        addAction(definition);
    }

    m_application.readSettings();
    m_shortcutRuntime->setup();
    updateShowMenuBarAction();
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
    m_shortcutRuntime->handleActionChanged(changedAction);
}

void ApplicationActionRuntime::updateShowMenuBarAction()
{
    if (m_showMenuBarAction == nullptr) {
        return;
    }

    const QSignalBlocker blocker(m_showMenuBarAction);
    m_showMenuBarAction->setChecked(menuPresentation() == KiriViewApplication::MenuBar);
}

}
