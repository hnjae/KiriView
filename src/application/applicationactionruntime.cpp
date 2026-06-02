// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionruntime.h"

#include "applicationshortcutruntime.h"
#include "kiriviewapplicationactions.h"

#include <KirigamiActionCollection>
#include <QIcon>

#include <utility>

namespace {
namespace Actions = KiriView::ApplicationActions;

Actions::VideoShortcutAvailabilityInput videoShortcutInput(bool helpShortcutsEnabled,
    bool viewerShortcutsEnabled, bool videoFileDeletionInProgress,
    bool videoDirectMediaNavigationActive)
{
    return Actions::VideoShortcutAvailabilityInput {
        helpShortcutsEnabled,
        viewerShortcutsEnabled,
        videoFileDeletionInProgress,
        videoDirectMediaNavigationActive,
    };
}
}

namespace KiriView::ApplicationActions {
ApplicationActionRuntime::ApplicationActionRuntime(ApplicationActionHost &host, Callbacks callbacks)
    : m_host(host)
    , m_actionRegistry(host)
    , m_menuPresentationRuntime(host, std::move(callbacks.menuPresentationChanged))
    , m_shortcutRuntime(std::make_unique<ApplicationShortcutRuntime>(
          m_host, m_actionRegistry, std::move(callbacks.shortcutRevisionChanged)))
    , m_actionStateChanged(std::move(callbacks.actionStateChanged))
    , m_actionTriggered(std::move(callbacks.actionTriggered))
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

int ApplicationActionRuntime::actionStateRevision() const { return m_actionStateRevision; }

bool ApplicationActionRuntime::actionPlacementEnabled(ActionId actionId) const
{
    return applicationActionState(actionId, m_actionStateInput).placementEnabled;
}

QString ApplicationActionRuntime::actionMenuText(ActionId actionId) const
{
    return applicationActionMenuText(actionId, m_actionStateInput);
}

QString ApplicationActionRuntime::actionToolbarText(ActionId actionId) const
{
    return applicationActionToolbarText(actionId);
}

QString ApplicationActionRuntime::actionToolbarTooltipText(ActionId actionId) const
{
    return applicationActionToolbarTooltipText(actionId);
}

bool ApplicationActionRuntime::videoActionUnsupported(ActionId actionId) const
{
    return ApplicationActions::videoActionUnsupported(actionId);
}

bool ApplicationActionRuntime::mediaHorizontalArrowShortcutsEnabled(bool videoMode,
    bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
    bool videoDirectMediaNavigationActive, bool videoFileDeletionInProgress) const
{
    return ApplicationActions::mediaHorizontalArrowShortcutsEnabled(videoMode,
        imageReadyViewerShortcutsEnabled,
        videoShortcutInput(false, videoViewerShortcutsEnabled, videoFileDeletionInProgress,
            videoDirectMediaNavigationActive));
}

void ApplicationActionRuntime::setActionStateInput(const ApplicationActionStateInput &input)
{
    m_actionStateInput = input;
    applyActionState();
    ++m_actionStateRevision;
    if (m_actionStateChanged) {
        m_actionStateChanged();
    }
}

void ApplicationActionRuntime::setupActions()
{
    m_host.mainActionCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

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

        if (registeredAction != nullptr
            && definition.shortcutConfigurability
                == Actions::ShortcutConfigurability::NonConfigurable) {
            KirigamiActionCollection::setShortcutsConfigurable(registeredAction, false);
        }
        m_actionRegistry.registerAction(definition, registeredAction);
        if (registeredAction != nullptr) {
            QObject::connect(registeredAction, &QAction::triggered, m_host.actionContext(),
                [this, actionId = definition.actionId, registeredAction]() {
                    handleActionTriggered(actionId, registeredAction);
                });
        }
    };

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        addAction(definition);
    }

    m_host.readActionSettings();
    m_menuPresentationRuntime.syncFromSettings();
    m_shortcutRuntime->setup();
    applyActionState();
}

QAction *ApplicationActionRuntime::addRegisteredAction(const QString &name, const QString &text,
    const QString &iconName, const QList<QKeySequence> &defaultShortcuts)
{
    auto *action = new QAction(m_host.actionContext());
    action->setObjectName(name);
    action->setText(text);
    if (!iconName.isEmpty()) {
        action->setIcon(QIcon::fromTheme(iconName));
    }

    m_host.mainActionCollection()->addAction(name, action);
    KirigamiActionCollection::setDefaultShortcuts(action, defaultShortcuts);
    QObject::connect(action, &QAction::changed, m_host.actionContext(),
        [this, action]() { handleActionChanged(action); });
    return action;
}

QAction *ApplicationActionRuntime::addStandardAction(KStandardActions::StandardAction actionType,
    const QString &name, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    QAction *action = m_host.mainActionCollection()->addAction(
        actionType, name, m_host.actionContext(), [](bool) { });
    return finishRegisteredAction(action, text, defaultShortcuts);
}

QAction *ApplicationActionRuntime::finishRegisteredAction(
    QAction *action, const QString &text, const QList<QKeySequence> &defaultShortcuts)
{
    action->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(action, defaultShortcuts);
    QObject::connect(action, &QAction::changed, m_host.actionContext(),
        [this, action]() { handleActionChanged(action); });
    return action;
}

void ApplicationActionRuntime::handleActionChanged(QAction *changedAction)
{
    if (m_applyingActionState) {
        return;
    }
    m_shortcutRuntime->handleActionChanged(changedAction);
}

void ApplicationActionRuntime::handleActionTriggered(ActionId actionId, QAction *triggeredAction)
{
    if (triggeredAction == nullptr || !triggeredAction->isEnabled()) {
        return;
    }
    if (m_actionTriggered) {
        m_actionTriggered(actionId);
    }
}

void ApplicationActionRuntime::applyActionState()
{
    m_applyingActionState = true;
    for (const RegisteredApplicationAction &registeredAction :
        m_actionRegistry.registeredActions()) {
        QAction *action = registeredAction.action;
        const ApplicationActionState state
            = applicationActionState(registeredAction.actionId, m_actionStateInput);
        action->setEnabled(state.actionEnabled);
        if (registeredAction.actionId == ActionId::OptionsShowMenubarAction) {
            continue;
        }
        action->setCheckable(state.checkable);
        if (state.checkable) {
            action->setChecked(state.checked);
        }
    }
    m_applyingActionState = false;
}

}
