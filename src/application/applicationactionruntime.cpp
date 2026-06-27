// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionruntime.h"

#include "applicationcommandportsource.h"
#include "applicationshortcutruntime.h"
#include "kiriviewapplicationactions.h"

#include <KirigamiActionCollection>
#include <QIcon>

#include <utility>

namespace {
namespace Actions = kiriview::ApplicationActions;

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

bool sharedImagePannabilityActionGate(
    kiriview::DocumentSessionActionAvailabilityFacts facts, bool viewportLocalPannable)
{
    return facts.imageReady && viewportLocalPannable;
}

ImageActionAvailabilityInput imageActionAvailabilityInput(
    const Actions::ApplicationActionStateSnapshot& snapshot)
{
    const kiriview::DocumentSessionActionAvailabilityFacts& facts
        = snapshot.sessionActionAvailability;
    return ImageActionAvailabilityInput {
        facts.imageReady,
        snapshot.fileDeletionInProgress,
        snapshot.helpDialogOpen,
        snapshot.textInputFocused,
        sharedImagePannabilityActionGate(facts, snapshot.imagePannable),
        facts.containerNavigationAvailable,
        facts.twoPageModeActive,
        facts.twoPageModeAvailable,
        facts.rightToLeftReadingActive,
        facts.rightToLeftReadingAvailable,
    };
}

Actions::ApplicationActionStateInput actionStateInput(
    const Actions::ApplicationActionStateSnapshot& snapshot,
    const ImageActionAvailabilityProjection& projection)
{
    const kiriview::DocumentSessionActionAvailabilityFacts& facts
        = snapshot.sessionActionAvailability;
    const bool activeNavigationActionsAvailable
        = snapshot.activeNavigationDispatchAvailable && projection.helpShortcutsEnabled;

    Actions::ApplicationActionStateInput input;
    input.uiGateRevision = snapshot.uiGateRevision;
    input.helpActionsEnabled = projection.helpShortcutsEnabled;
    input.readyActionsEnabled = projection.canUseReadyActions;
    input.rotateActionsEnabled = projection.canUseRotateActions;
    input.twoPageModeActionsEnabled = projection.canUseTwoPageModeActions;
    input.rightToLeftReadingActionsEnabled = projection.canUseRightToLeftReadingActions;
    input.containerNavigationActionsEnabled = projection.containerShortcutsEnabled;
    input.displayedMediaOpenWithAvailable = snapshot.displayedMediaOpenWithAvailable;
    input.displayedFileDeletionAvailable = snapshot.displayedFileDeletionAvailable;
    input.fileDeletionInProgress = snapshot.fileDeletionInProgress;
    input.activeNavigationAvailable = snapshot.activeNavigationAvailable;
    input.activeNavigationKnown = snapshot.activeNavigationKnown;
    input.activeNavigationHasTargets = snapshot.activeNavigationHasTargets;
    input.canOpenPreviousActiveNavigation = snapshot.canOpenPreviousActiveNavigation;
    input.canOpenNextActiveNavigation = snapshot.canOpenNextActiveNavigation;
    input.fitModeSelected = facts.fitModeSelected;
    input.fitHeightModeSelected = facts.fitHeightModeSelected;
    input.fitWidthModeSelected = facts.fitWidthModeSelected;
    input.twoPageModeActive = projection.twoPageModeActive;
    input.rightToLeftReadingActive = projection.rightToLeftReadingActive;
    input.infoPanelVisible = snapshot.infoPanelVisible;
    input.thumbnailPanelVisible = snapshot.thumbnailPanelVisible;
    input.fullscreen = snapshot.fullscreen;
    input.applicationMenuShortcutEnabled = snapshot.applicationMenuShortcutEnabled;
    input.showMenubarActionEnabled = snapshot.showMenubarActionEnabled;
    input.directMediaNavigationBoundaryActive = snapshot.directMediaNavigationBoundaryActive;
    input.viewerShortcutsEnabled = projection.viewerShortcutsEnabled;
    input.readyShortcutsEnabled = projection.readyShortcutsEnabled;
    input.readyViewerShortcutsEnabled = projection.readyViewerShortcutsEnabled;
    input.twoPageViewerShortcutsEnabled = projection.twoPageViewerShortcutsEnabled;
    input.rightToLeftReadingShortcutsEnabled = projection.rightToLeftReadingShortcutsEnabled;
    input.rightToLeftReadingViewerShortcutsEnabled
        = projection.rightToLeftReadingViewerShortcutsEnabled;
    input.rotateShortcutsEnabled = projection.rotateShortcutsEnabled;
    input.rotateViewerShortcutsEnabled = projection.rotateViewerShortcutsEnabled;
    input.pannableShortcutsEnabled = projection.pannableShortcutsEnabled;
    input.pannableViewerShortcutsEnabled = projection.pannableViewerShortcutsEnabled;
    input.containerViewerShortcutsEnabled = projection.containerViewerShortcutsEnabled;
    input.activeNavigationActionsAvailable = activeNavigationActionsAvailable;
    input.videoMode = snapshot.videoMode;
    input.videoFileDeletionInProgress = snapshot.fileDeletionInProgress;
    input.videoSeekable = snapshot.videoSeekable;
    input.videoDuration = snapshot.videoDuration;
    return input;
}

Actions::ApplicationCommandRouterInput routerInputForSnapshot(
    const Actions::ApplicationActionStateSnapshot& snapshot,
    const ImageActionAvailabilityProjection& projection)
{
    Actions::ApplicationCommandRouterInput input;
    input.imagePannable = snapshot.imagePannable;
    input.rightToLeftReadingActive = projection.rightToLeftReadingActive;
    input.videoMode = snapshot.videoMode;
    input.imageDocumentPageNavigationActive = snapshot.imageDocumentPageNavigationActive;
    input.atKnownFirstActiveNavigation = snapshot.atKnownFirstActiveNavigation;
    input.canOpenPreviousActiveNavigation = snapshot.canOpenPreviousActiveNavigation;
    return input;
}
}

namespace kiriview::ApplicationActions {
ApplicationActionRuntime::ApplicationActionRuntime(ApplicationActionHost& host, Callbacks callbacks)
    : m_host(host)
    , m_actionRegistry(host)
    , m_menuPresentationRuntime(host, std::move(callbacks.menuPresentationChanged))
    , m_shortcutRuntime(std::make_unique<ApplicationShortcutRuntime>(m_host, m_actionRegistry,
          std::move(callbacks.shortcutRevisionChanged),
          ApplicationShortcutRuntime::TriggerCallbacks {
              std::move(callbacks.unsupportedVideoActionTriggered),
              std::move(callbacks.unsupportedImageActionTriggered),
              [this](bool leftArrow) { return executeHorizontalArrowShortcut(leftArrow); },
              [this](bool leftArrow) { return executeSinglePageArrowShortcut(leftArrow); },
              [this](bool up) { return executeVerticalPanShortcut(up); },
              [this](
                  qint64 deltaMilliseconds) { return executeVideoSeekShortcut(deltaMilliseconds); },
          }))
    , m_actionStateChanged(std::move(callbacks.actionStateChanged))
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

QAbstractListModel* ApplicationActionRuntime::shortcutHelpModel() const
{
    return m_shortcutRuntime->shortcutHelpModel();
}

QAbstractListModel* ApplicationActionRuntime::shortcutRouteModel() const
{
    return m_shortcutRuntime->shortcutRouteModel();
}

QAction* ApplicationActionRuntime::action(const QString& actionName)
{
    return m_actionRegistry.action(actionName);
}

QAction* ApplicationActionRuntime::actionForId(ActionId actionId)
{
    return m_actionRegistry.actionForId(actionId);
}

QString ApplicationActionRuntime::actionName(ActionId actionId) const
{
    return m_actionRegistry.actionName(actionId);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjection(
    const QString& actionName) const
{
    return m_shortcutRuntime->shortcutProjection(actionName);
}

ApplicationShortcutProjection ApplicationActionRuntime::shortcutProjectionForId(
    ActionId actionId) const
{
    return m_shortcutRuntime->shortcutProjectionForId(actionId);
}

QList<QKeySequence> ApplicationActionRuntime::programWideShortcuts(const QString& actionName) const
{
    return m_shortcutRuntime->programWideShortcuts(actionName);
}

QList<QKeySequence> ApplicationActionRuntime::programWideShortcutsForId(ActionId actionId) const
{
    return m_shortcutRuntime->programWideShortcutsForId(actionId);
}

QList<QKeySequence> ApplicationActionRuntime::viewerLocalShortcuts(const QString& actionName) const
{
    return m_shortcutRuntime->viewerLocalShortcuts(actionName);
}

QList<QKeySequence> ApplicationActionRuntime::viewerLocalShortcutsForId(ActionId actionId) const
{
    return m_shortcutRuntime->viewerLocalShortcutsForId(actionId);
}

bool ApplicationActionRuntime::setViewerLocalShortcuts(
    const QString& actionName, const QList<QKeySequence>& shortcuts)
{
    return m_shortcutRuntime->setViewerLocalShortcuts(actionName, shortcuts);
}

bool ApplicationActionRuntime::setViewerLocalShortcutsForId(
    ActionId actionId, const QList<QKeySequence>& shortcuts)
{
    return m_shortcutRuntime->setViewerLocalShortcutsForId(actionId, shortcuts);
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

bool ApplicationActionRuntime::imageActionUnsupported(ActionId actionId) const
{
    return ApplicationActions::imageActionUnsupported(actionId);
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

void ApplicationActionRuntime::setActionStateSnapshot(
    const ApplicationActionStateSnapshot& snapshot)
{
    m_actionStateSnapshot = snapshot;
    m_imageActionProjection
        = imageActionAvailabilityProjection(imageActionAvailabilityInput(snapshot));
    setActionStateInput(actionStateInput(snapshot, m_imageActionProjection));
}

void ApplicationActionRuntime::setActionStateInput(const ApplicationActionStateInput& input)
{
    m_actionStateInput = input;
    applyActionState();
    m_shortcutRuntime->setActionStateInput(m_actionStateInput);
    ++m_actionStateRevision;
    if (m_actionStateChanged) {
        m_actionStateChanged();
    }
}

void ApplicationActionRuntime::setCommandPortSource(ApplicationCommandPortSource* source)
{
    m_commandPortSource = source;
}

ApplicationCommandRouterInput ApplicationActionRuntime::commandRouterInput() const
{
    return routerInputForSnapshot(m_actionStateSnapshot, m_imageActionProjection);
}

bool ApplicationActionRuntime::rightToLeftReadingActive() const
{
    return m_imageActionProjection.rightToLeftReadingActive;
}

NavigationPresentationProjection ApplicationActionRuntime::navigationPresentationProjection() const
{
    return kiriview::ApplicationActions::navigationPresentationProjection(
        m_imageActionProjection.rightToLeftReadingActive);
}

void ApplicationActionRuntime::handleActionTriggered(ActionId actionId) const
{
    m_commandRouter.handleActionTriggered(actionId, commandRouterInput(), commandRouterPorts());
}

void ApplicationActionRuntime::handleScanForwardAction() const
{
    m_commandRouter.handleScanForwardAction(commandRouterInput(), commandRouterPorts());
}

void ApplicationActionRuntime::handleScanBackwardAction() const
{
    m_commandRouter.handleScanBackwardAction(commandRouterInput(), commandRouterPorts());
}

bool ApplicationActionRuntime::executeHorizontalArrowShortcut(bool leftArrow) const
{
    return m_commandRouter.executeHorizontalArrowShortcut(
        commandRouterInput(), commandRouterPorts(), leftArrow);
}

bool ApplicationActionRuntime::executeSinglePageArrowShortcut(bool leftArrow) const
{
    return m_commandRouter.executeSinglePageArrowShortcut(
        commandRouterInput(), commandRouterPorts(), leftArrow);
}

bool ApplicationActionRuntime::executeVerticalPanShortcut(bool up) const
{
    return m_commandRouter.executeVerticalPanShortcut(
        commandRouterInput(), commandRouterPorts(), up);
}

bool ApplicationActionRuntime::executeVideoSeekShortcut(qint64 deltaMilliseconds) const
{
    return m_commandRouter.executeVideoSeekShortcut(
        commandRouterInput(), commandRouterPorts(), deltaMilliseconds);
}

void ApplicationActionRuntime::setShortcutHost(QObject* host)
{
    m_shortcutRuntime->setShortcutHost(host);
}

void ApplicationActionRuntime::setupActions()
{
    m_host.mainActionCollection()->setComponentDisplayName(QStringLiteral("KiriView"));

    const auto addAction = [this](const Actions::ActionDefinition& definition) {
        const QString name = QString::fromLatin1(definition.name);
        const QList<QKeySequence> shortcuts
            = Actions::defaultShortcuts(definition.defaultProgramWideShortcuts);
        QAction* registeredAction = nullptr;

        switch (definition.kind) {
        case Actions::RegistrationKind::Existing:
            if (QAction* action = m_actionRegistry.collectionAction(name)) {
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

    for (const Actions::ActionDefinition& definition : Actions::definitions()) {
        addAction(definition);
    }

    m_host.readActionSettings();
    m_menuPresentationRuntime.syncFromSettings();
    m_shortcutRuntime->setup();
    applyActionState();
    m_shortcutRuntime->setActionStateInput(m_actionStateInput);
}

QAction* ApplicationActionRuntime::addRegisteredAction(const QString& name, const QString& text,
    const QString& iconName, const QList<QKeySequence>& defaultShortcuts)
{
    auto* action = new QAction(m_host.actionContext());
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

QAction* ApplicationActionRuntime::addStandardAction(KStandardActions::StandardAction actionType,
    const QString& name, const QString& text, const QList<QKeySequence>& defaultShortcuts)
{
    QAction* action = m_host.mainActionCollection()->addAction(
        actionType, name, m_host.actionContext(), [](bool) { });
    return finishRegisteredAction(action, text, defaultShortcuts);
}

QAction* ApplicationActionRuntime::finishRegisteredAction(
    QAction* action, const QString& text, const QList<QKeySequence>& defaultShortcuts)
{
    action->setText(text);
    KirigamiActionCollection::setDefaultShortcuts(action, defaultShortcuts);
    QObject::connect(action, &QAction::changed, m_host.actionContext(),
        [this, action]() { handleActionChanged(action); });
    return action;
}

void ApplicationActionRuntime::handleActionChanged(QAction* changedAction)
{
    if (m_applyingActionState) {
        return;
    }
    m_shortcutRuntime->handleActionChanged(changedAction);
}

void ApplicationActionRuntime::handleActionTriggered(ActionId actionId, QAction* triggeredAction)
{
    if (triggeredAction == nullptr || !triggeredAction->isEnabled()) {
        return;
    }
    handleActionTriggered(actionId);
}

void ApplicationActionRuntime::applyActionState()
{
    m_applyingActionState = true;
    for (const RegisteredApplicationAction& registeredAction :
        m_actionRegistry.registeredActions()) {
        QAction* action = registeredAction.action;
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

ApplicationCommandRouterPorts ApplicationActionRuntime::commandRouterPorts() const
{
    if (m_commandPortSource == nullptr) {
        return {};
    }
    return applicationCommandRouterPorts(*m_commandPortSource);
}

}
