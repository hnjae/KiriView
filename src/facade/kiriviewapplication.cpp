// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriviewapplication.h"

#include "application/applicationactionhost.h"
#include "application/applicationactionruntime.h"

namespace Actions = KiriView::ApplicationActions;

namespace KiriView::ApplicationActions {
class KiriViewApplicationActionHost final : public ApplicationActionHost
{
public:
    explicit KiriViewApplicationActionHost(KiriViewApplication &application)
        : m_application(application)
    {
    }

    QObject *actionContext() override { return &m_application; }
    KirigamiActionCollection *mainActionCollection() override
    {
        return m_application.applicationMainActionCollection();
    }
    QAction *inheritedAction(const QString &actionName) override
    {
        return m_application.inheritedApplicationAction(actionName);
    }
    void readActionSettings() override { m_application.readApplicationActionSettings(); }

private:
    KiriViewApplication &m_application;
};
}

static_assert(static_cast<int>(Actions::MenuPresentation::HamburgerMenu)
    == static_cast<int>(KiriViewApplication::HamburgerMenu));
static_assert(static_cast<int>(Actions::MenuPresentation::MenuBar)
    == static_cast<int>(KiriViewApplication::MenuBar));
static_assert(static_cast<int>(Actions::ActionId::FileOpenAction)
    == static_cast<int>(KiriViewApplication::FileOpenAction));
static_assert(static_cast<int>(Actions::ActionId::ActionCount)
    == static_cast<int>(KiriViewApplication::ActionCount));

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
    , m_actionHost(std::make_unique<Actions::KiriViewApplicationActionHost>(*this))
    , m_actionRuntime(std::make_unique<Actions::ApplicationActionRuntime>(*m_actionHost,
          Actions::ApplicationActionRuntime::Callbacks {
              [this]() { Q_EMIT menuPresentationChanged(); },
              [this]() { Q_EMIT shortcutRevisionChanged(); },
          }))
{
    KiriViewApplication::setupActions();
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return facadeMenuPresentation(m_actionRuntime->menuPresentation());
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    m_actionRuntime->setMenuPresentation(domainMenuPresentation(presentation));
}

int KiriViewApplication::shortcutRevision() const { return m_actionRuntime->shortcutRevision(); }

QAbstractListModel *KiriViewApplication::shortcutHelpModel() const
{
    return m_actionRuntime->shortcutHelpModel();
}

QAbstractListModel *KiriViewApplication::shortcutRouteModel() const
{
    return m_actionRuntime->shortcutRouteModel();
}

Actions::MenuPresentation KiriViewApplication::domainMenuPresentation(MenuPresentation presentation)
{
    if (presentation == MenuBar) {
        return Actions::MenuPresentation::MenuBar;
    }

    return Actions::MenuPresentation::HamburgerMenu;
}

KiriViewApplication::MenuPresentation KiriViewApplication::facadeMenuPresentation(
    Actions::MenuPresentation presentation)
{
    if (presentation == Actions::MenuPresentation::MenuBar) {
        return MenuBar;
    }

    return HamburgerMenu;
}

Actions::ActionId KiriViewApplication::domainActionId(ActionId actionId)
{
    return static_cast<Actions::ActionId>(static_cast<int>(actionId));
}

KiriViewApplication::ActionId KiriViewApplication::facadeActionId(Actions::ActionId actionId)
{
    return static_cast<ActionId>(static_cast<int>(actionId));
}

QAction *KiriViewApplication::action(const QString &actionName)
{
    return m_actionRuntime->action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    return m_actionRuntime->actionForId(domainActionId(actionId));
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return m_actionRuntime->actionName(domainActionId(actionId));
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId))
        .shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId))
        .shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutAliases(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutAliases;
}

QList<QKeySequence> KiriViewApplication::shortcutAliasesForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcutAliases;
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutText;
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).shortcutText;
}

QKeySequence KiriViewApplication::menuShortcut(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcut;
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).menuShortcut;
}

QString KiriViewApplication::menuShortcutText(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcutText;
}

QString KiriViewApplication::menuShortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(domainActionId(actionId)).menuShortcutText;
}

bool KiriViewApplication::videoActionUnsupported(ActionId actionId) const
{
    return m_actionRuntime->videoActionUnsupported(domainActionId(actionId));
}

bool KiriViewApplication::mediaHorizontalArrowShortcutsEnabled(bool videoMode,
    bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
    bool videoMediaNavigationActive, bool videoFileDeletionInProgress) const
{
    return m_actionRuntime->mediaHorizontalArrowShortcutsEnabled(videoMode,
        imageReadyViewerShortcutsEnabled, videoViewerShortcutsEnabled, videoMediaNavigationActive,
        videoFileDeletionInProgress);
}

void KiriViewApplication::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    if (m_actionRuntime != nullptr) {
        m_actionRuntime->setupActions();
    }
}

KirigamiActionCollection *KiriViewApplication::applicationMainActionCollection()
{
    return mainCollection();
}

QAction *KiriViewApplication::inheritedApplicationAction(const QString &actionName)
{
    return AbstractKirigamiApplication::action(actionName);
}

void KiriViewApplication::readApplicationActionSettings() { readSettings(); }
