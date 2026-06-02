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
#include <QString>
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
    int actionStateRevision() const;
    bool actionPlacementEnabled(ActionId actionId) const;
    bool videoActionUnsupported(ActionId actionId) const;
    bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
        bool videoViewerShortcutsEnabled, bool videoDirectMediaNavigationActive,
        bool videoFileDeletionInProgress) const;
    void setActionStateInput(const ApplicationActionStateInput &input);

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

    ApplicationActionHost &m_host;
    ApplicationActionRegistry m_actionRegistry;
    ApplicationMenuPresentationRuntime m_menuPresentationRuntime;
    std::unique_ptr<ApplicationShortcutRuntime> m_shortcutRuntime;
    ApplicationActionStateInput m_actionStateInput;
    int m_actionStateRevision = 0;
    bool m_applyingActionState = false;
    std::function<void()> m_actionStateChanged;
};
}

#endif
