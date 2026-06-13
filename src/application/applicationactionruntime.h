// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONRUNTIME_H
#define KIRIVIEW_APPLICATIONACTIONRUNTIME_H

#include "applicationactionhost.h"
#include "applicationactionregistry.h"
#include "applicationactionstatepolicy.h"
#include "applicationmenupresentationruntime.h"
#include "applicationshortcutpolicy.h"

#include <KStandardActions>
#include <QAbstractListModel>
#include <QAction>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <functional>
#include <memory>

namespace KiriView::ApplicationActions {
struct ActionDefinition;
class ApplicationShortcutRuntime;

class ApplicationActionRuntime final
{
public:
    struct Callbacks {
        std::function<void()> menuPresentationChanged;
        std::function<void()> shortcutRevisionChanged;
        std::function<void()> actionStateChanged;
        std::function<void(ActionId)> actionTriggered;
        std::function<void(ActionId)> unsupportedVideoActionTriggered;
        std::function<void(ActionId)> unsupportedImageActionTriggered;
        std::function<bool(bool)> horizontalArrowShortcutTriggered;
        std::function<bool(bool)> singlePageArrowShortcutTriggered;
        std::function<bool(bool)> verticalPanShortcutTriggered;
        std::function<bool(qint64)> videoSeekShortcutTriggered;
    };

    explicit ApplicationActionRuntime(ApplicationActionHost &host, Callbacks callbacks = {});
    ~ApplicationActionRuntime();

    MenuPresentation menuPresentation() const;
    void setMenuPresentation(MenuPresentation presentation);
    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;
    QAbstractListModel *shortcutRouteModel() const;

    QAction *action(const QString &actionName);
    QAction *actionForId(ActionId actionId);
    QString actionName(ActionId actionId) const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(ActionId actionId) const;
    QList<QKeySequence> programWideShortcuts(const QString &actionName) const;
    QList<QKeySequence> programWideShortcutsForId(ActionId actionId) const;
    QList<QKeySequence> viewerLocalShortcuts(const QString &actionName) const;
    QList<QKeySequence> viewerLocalShortcutsForId(ActionId actionId) const;
    bool setViewerLocalShortcuts(const QString &actionName, const QList<QKeySequence> &shortcuts);
    bool setViewerLocalShortcutsForId(ActionId actionId, const QList<QKeySequence> &shortcuts);
    int actionStateRevision() const;
    bool actionPlacementEnabled(ActionId actionId) const;
    QString actionMenuText(ActionId actionId) const;
    QString actionToolbarText(ActionId actionId) const;
    QString actionToolbarTooltipText(ActionId actionId) const;
    bool videoActionUnsupported(ActionId actionId) const;
    bool imageActionUnsupported(ActionId actionId) const;
    bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
        bool videoViewerShortcutsEnabled, bool videoDirectMediaNavigationActive,
        bool videoFileDeletionInProgress) const;
    void setActionStateInput(const ApplicationActionStateInput &input);
    void setShortcutHost(QObject *host);

    void setupActions();

private:
    QAction *addRegisteredAction(const QString &name, const QString &text, const QString &iconName,
        const QList<QKeySequence> &defaultShortcuts = {});
    QAction *addStandardAction(KStandardActions::StandardAction actionType, const QString &name,
        const QString &text, const QList<QKeySequence> &defaultShortcuts);
    QAction *finishRegisteredAction(QAction *registeredAction, const QString &text,
        const QList<QKeySequence> &defaultShortcuts);
    void applyActionState();
    void handleActionChanged(QAction *changedAction);
    void handleActionTriggered(ActionId actionId, QAction *triggeredAction);

    ApplicationActionHost &m_host;
    ApplicationActionRegistry m_actionRegistry;
    ApplicationMenuPresentationRuntime m_menuPresentationRuntime;
    std::unique_ptr<ApplicationShortcutRuntime> m_shortcutRuntime;
    ApplicationActionStateInput m_actionStateInput;
    int m_actionStateRevision = 0;
    bool m_applyingActionState = false;
    std::function<void()> m_actionStateChanged;
    std::function<void(ActionId)> m_actionTriggered;
};
}

#endif
