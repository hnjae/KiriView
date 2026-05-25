// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionruntime.h"

#include "applicationshortcutruntime.h"
#include "facade/kiriviewapplication.h"
#include "kiriviewapplicationactions.h"

#include <KirigamiActionCollection>
#include <QIcon>

#include <optional>
#include <utility>

namespace {
namespace Actions = KiriView::ApplicationActions;

Actions::VideoShortcutAvailabilityInput videoShortcutInput(bool helpShortcutsEnabled,
    bool viewerShortcutsEnabled, bool videoFileDeletionInProgress, bool videoMediaNavigationActive)
{
    return Actions::VideoShortcutAvailabilityInput {
        helpShortcutsEnabled,
        viewerShortcutsEnabled,
        videoFileDeletionInProgress,
        videoMediaNavigationActive,
    };
}
}

namespace KiriView::ApplicationActions {
ApplicationActionRuntime::ApplicationActionRuntime(
    KiriViewApplication &application, Callbacks callbacks)
    : m_application(application)
    , m_actionRegistry(application)
    , m_menuPresentationRuntime(application, std::move(callbacks.menuPresentationChanged))
    , m_shortcutRuntime(std::make_unique<ApplicationShortcutRuntime>(
          m_application, m_actionRegistry, std::move(callbacks.shortcutRevisionChanged)))
{
}

ApplicationActionRuntime::~ApplicationActionRuntime() = default;

MenuPresentation ApplicationActionRuntime::menuPresentation() const
{
    return m_menuPresentationRuntime.menuPresentation();
}

void ApplicationActionRuntime::setMenuPresentation(MenuPresentation presentation)
{
    m_menuPresentationRuntime.setMenuPresentation(presentation);
}

int ApplicationActionRuntime::shortcutRevision() const
{
    return m_shortcutRuntime->shortcutRevision();
}

QAbstractListModel *ApplicationActionRuntime::shortcutHelpModel() const
{
    return m_shortcutRuntime->shortcutHelpModel();
}

QAbstractListModel *ApplicationActionRuntime::shortcutRouteModel() const
{
    return m_shortcutRuntime->shortcutRouteModel();
}

QAction *ApplicationActionRuntime::action(const QString &actionName)
{
    return m_actionRegistry.action(actionName);
}

QAction *ApplicationActionRuntime::actionForId(ActionId actionId)
{
    return m_actionRegistry.actionForId(actionId);
}

QString ApplicationActionRuntime::actionName(ActionId actionId) const
{
    return m_actionRegistry.actionName(actionId);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjection(
    const QString &actionName) const
{
    return m_shortcutRuntime->shortcutProjection(actionName);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjectionForId(
    ActionId actionId) const
{
    return m_shortcutRuntime->shortcutProjectionForId(actionId);
}

bool ApplicationActionRuntime::videoShortcutsEnabledForScope(int shortcutScope,
    bool helpShortcutsEnabled, bool viewerShortcutsEnabled, bool videoFileDeletionInProgress,
    bool videoMediaNavigationActive) const
{
    const std::optional<ImageShortcutScope> scope = imageShortcutScopeFromValue(shortcutScope);
    if (!scope.has_value()) {
        return false;
    }

    return ApplicationActions::videoShortcutsEnabledForScope(
        videoShortcutInput(helpShortcutsEnabled, viewerShortcutsEnabled,
            videoFileDeletionInProgress, videoMediaNavigationActive),
        *scope);
}

bool ApplicationActionRuntime::videoActionUnsupported(ActionId actionId) const
{
    return ApplicationActions::videoActionUnsupported(actionId);
}

bool ApplicationActionRuntime::mediaHorizontalArrowShortcutsEnabled(bool videoMode,
    bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
    bool videoMediaNavigationActive, bool videoFileDeletionInProgress) const
{
    return ApplicationActions::mediaHorizontalArrowShortcutsEnabled(videoMode,
        imageReadyViewerShortcutsEnabled,
        videoShortcutInput(false, videoViewerShortcutsEnabled, videoFileDeletionInProgress,
            videoMediaNavigationActive));
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
            registeredAction = addStandardAction(
                definition.actionType, name, Actions::localizedString(definition.text), shortcuts);
            m_menuPresentationRuntime.bindShowMenuBarAction(registeredAction);
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
    m_menuPresentationRuntime.syncFromSettings();
    m_shortcutRuntime->setup();
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

}
