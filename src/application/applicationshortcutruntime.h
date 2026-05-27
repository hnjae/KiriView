// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H
#define KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H

#include "applicationactionhost.h"
#include "applicationactionregistry.h"
#include "applicationshortcutpolicy.h"

#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <functional>
#include <memory>

namespace KiriView::ApplicationActions {
class ShortcutRouteModel;
class ShortcutHelpModel;
struct ShortcutHelpRow;

class ApplicationShortcutRuntime final
{
public:
    using ChangeCallback = std::function<void()>;

    ApplicationShortcutRuntime(ApplicationActionHost &host,
        const ApplicationActionRegistry &actionRegistry, ChangeCallback changeCallback = {});
    ~ApplicationShortcutRuntime();

    void setup();
    void handleActionChanged(QAction *changedAction);

    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;
    QAbstractListModel *shortcutRouteModel() const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(ActionId actionId) const;

private:
    void sanitizeActionShortcuts();
    void sanitizeActionShortcuts(QAction *action);
    ApplicationShortcutProjection shortcutProjectionForAction(
        const QAction *action, ShortcutAliasPolicy aliasPolicy) const;
    static QString actionDisplayText(const QAction *action);
    static QString shortcutDisplayText(const QAction *action);
    QList<ShortcutHelpRow> shortcutHelpRows() const;

    ApplicationActionHost &m_host;
    const ApplicationActionRegistry &m_actionRegistry;
    ChangeCallback m_changeCallback;
    std::unique_ptr<ShortcutHelpModel> m_shortcutHelpModel;
    std::unique_ptr<ShortcutRouteModel> m_shortcutRouteModel;
    int m_shortcutRevision = 0;
    bool m_sanitizingShortcuts = false;
};
}

#endif
