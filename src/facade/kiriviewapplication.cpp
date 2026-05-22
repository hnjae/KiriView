// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriviewapplication.h"

#include "application/applicationactionruntime.h"

namespace Actions = KiriView::ApplicationActions;

KiriViewApplication::KiriViewApplication(QObject *parent)
    : AbstractKirigamiApplication(parent)
    , m_actionRuntime(std::make_unique<Actions::ApplicationActionRuntime>(*this))
{
    KiriViewApplication::setupActions();
}

KiriViewApplication::~KiriViewApplication() = default;

KiriViewApplication::MenuPresentation KiriViewApplication::menuPresentation() const
{
    return m_actionRuntime->menuPresentation();
}

void KiriViewApplication::setMenuPresentation(MenuPresentation presentation)
{
    m_actionRuntime->setMenuPresentation(presentation);
}

int KiriViewApplication::shortcutRevision() const { return m_actionRuntime->shortcutRevision(); }

QAbstractListModel *KiriViewApplication::shortcutHelpModel() const
{
    return m_actionRuntime->shortcutHelpModel();
}

QAction *KiriViewApplication::action(const QString &actionName)
{
    return m_actionRuntime->action(actionName);
}

QAction *KiriViewApplication::actionForId(ActionId actionId)
{
    return m_actionRuntime->actionForId(actionId);
}

QString KiriViewApplication::actionName(ActionId actionId) const
{
    return m_actionRuntime->actionName(actionId);
}

QList<QKeySequence> KiriViewApplication::shortcuts(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).shortcuts;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithCommandModifierForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).shortcutsWithCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifier(
    const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutsWithoutCommandModifierForId(
    ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).shortcutsWithoutCommandModifier;
}

QList<QKeySequence> KiriViewApplication::shortcutAliases(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutAliases;
}

QList<QKeySequence> KiriViewApplication::shortcutAliasesForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).shortcutAliases;
}

QString KiriViewApplication::shortcutText(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).shortcutText;
}

QString KiriViewApplication::shortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).shortcutText;
}

QKeySequence KiriViewApplication::menuShortcut(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcut;
}

QKeySequence KiriViewApplication::menuShortcutForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).menuShortcut;
}

QString KiriViewApplication::menuShortcutText(const QString &actionName) const
{
    return m_actionRuntime->shortcutProjection(actionName).menuShortcutText;
}

QString KiriViewApplication::menuShortcutTextForId(ActionId actionId) const
{
    return m_actionRuntime->shortcutProjectionForId(actionId).menuShortcutText;
}

QVariantList KiriViewApplication::shortcutRoutes() const
{
    return m_actionRuntime->shortcutRoutes();
}

bool KiriViewApplication::videoShortcutsEnabledForScope(int shortcutScope,
    bool helpShortcutsEnabled, bool viewerShortcutsEnabled, bool videoFileDeletionInProgress,
    bool videoMediaNavigationActive) const
{
    return m_actionRuntime->videoShortcutsEnabledForScope(shortcutScope, helpShortcutsEnabled,
        viewerShortcutsEnabled, videoFileDeletionInProgress, videoMediaNavigationActive);
}

bool KiriViewApplication::videoActionUnsupported(ActionId actionId) const
{
    return m_actionRuntime->videoActionUnsupported(actionId);
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
