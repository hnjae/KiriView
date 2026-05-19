// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H
#define KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H

#include "applicationactionregistry.h"
#include "applicationshortcutpolicy.h"
#include "kiriviewapplication.h"

#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <memory>

namespace KiriView::ApplicationActions {
class ShortcutHelpModel;
struct ShortcutHelpRow;

class ApplicationShortcutRuntime final
{
public:
    ApplicationShortcutRuntime(
        KiriViewApplication &application, const ApplicationActionRegistry &actionRegistry);
    ~ApplicationShortcutRuntime();

    void setup();
    void handleActionChanged(QAction *changedAction);

    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(
        KiriViewApplication::ActionId actionId) const;

private:
    void sanitizeActionShortcuts();
    void sanitizeActionShortcuts(QAction *action);
    ApplicationShortcutProjection shortcutProjectionForAction(const QAction *action) const;
    static QString actionDisplayText(const QAction *action);
    static QString shortcutDisplayText(const QAction *action);
    QList<ShortcutHelpRow> shortcutHelpRows() const;

    KiriViewApplication &m_application;
    const ApplicationActionRegistry &m_actionRegistry;
    std::unique_ptr<ShortcutHelpModel> m_shortcutHelpModel;
    int m_shortcutRevision = 0;
    bool m_sanitizingShortcuts = false;
};
}

#endif
