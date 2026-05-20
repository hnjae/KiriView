// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONRUNTIME_H
#define KIRIVIEW_APPLICATIONACTIONRUNTIME_H

#include "applicationactionregistry.h"
#include "applicationmenupresentationruntime.h"
#include "applicationshortcutpolicy.h"
#include "facade/kiriviewapplication.h"

#include <KStandardActions>
#include <QAbstractListModel>
#include <QAction>
#include <QList>
#include <QString>
#include <memory>

namespace KiriView::ApplicationActions {
struct ActionDefinition;
class ApplicationShortcutRuntime;

class ApplicationActionRuntime final
{
public:
    explicit ApplicationActionRuntime(KiriViewApplication &application);
    ~ApplicationActionRuntime();

    KiriViewApplication::MenuPresentation menuPresentation() const;
    void setMenuPresentation(KiriViewApplication::MenuPresentation presentation);
    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;

    QAction *action(const QString &actionName);
    QAction *actionForId(KiriViewApplication::ActionId actionId);
    QString actionName(KiriViewApplication::ActionId actionId) const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(
        KiriViewApplication::ActionId actionId) const;

    void setupActions();

private:
    QAction *addRegisteredAction(const QString &name, const QString &text, const QString &iconName,
        const QList<QKeySequence> &defaultShortcuts = {});
    QAction *addStandardAction(KStandardActions::StandardAction actionType, const QString &name,
        const QString &text, const QList<QKeySequence> &defaultShortcuts);
    QAction *finishRegisteredAction(QAction *registeredAction, const QString &text,
        const QList<QKeySequence> &defaultShortcuts);
    void handleActionChanged(QAction *changedAction);

    KiriViewApplication &m_application;
    ApplicationActionRegistry m_actionRegistry;
    ApplicationMenuPresentationRuntime m_menuPresentationRuntime;
    std::unique_ptr<ApplicationShortcutRuntime> m_shortcutRuntime;
};
}

#endif
