// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H
#define KIRIVIEW_APPLICATIONSHORTCUTRUNTIME_H

#include "applicationactionhost.h"
#include "applicationactionregistry.h"
#include "applicationactionstatepolicy.h"
#include "applicationshortcutpolicy.h"

#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWindow>
#include <QtGlobal>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace kiriview::ApplicationActions {
class ShortcutRouteModel;
class ShortcutHelpModel;
struct ShortcutHelpRow;

class ApplicationShortcutRuntime final
{
public:
    using ChangeCallback = std::function<void()>;
    struct TriggerCallbacks {
        std::function<void(ActionId)> unsupportedVideoActionTriggered;
        std::function<void(ActionId)> unsupportedImageActionTriggered;
        std::function<bool(bool)> horizontalArrowShortcutTriggered;
        std::function<bool(bool)> singlePageArrowShortcutTriggered;
        std::function<bool(bool)> verticalPanShortcutTriggered;
        std::function<bool(qint64)> videoSeekShortcutTriggered;
    };

    ApplicationShortcutRuntime(ApplicationActionHost &host,
        const ApplicationActionRegistry &actionRegistry, ChangeCallback changeCallback = {},
        TriggerCallbacks triggerCallbacks = {});
    ~ApplicationShortcutRuntime();

    void setup();
    void handleActionChanged(QAction *changedAction);
    void setActionStateInput(const ApplicationActionStateInput &input);
    void setShortcutHost(QObject *host);

    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;
    QAbstractListModel *shortcutRouteModel() const;
    ApplicationShortcutProjection shortcutProjection(const QString &actionName) const;
    ApplicationShortcutProjection shortcutProjectionForId(ActionId actionId) const;
    QList<QKeySequence> programWideShortcuts(const QString &actionName) const;
    QList<QKeySequence> programWideShortcutsForId(ActionId actionId) const;
    QList<QKeySequence> viewerLocalShortcuts(const QString &actionName) const;
    QList<QKeySequence> viewerLocalShortcutsForId(ActionId actionId) const;
    bool setViewerLocalShortcuts(const QString &actionName, const QList<QKeySequence> &shortcuts);
    bool setViewerLocalShortcutsForId(ActionId actionId, const QList<QKeySequence> &shortcuts);

private:
    void loadViewerLocalShortcuts();
    void persistViewerLocalShortcuts(ActionId actionId) const;
    void notifyShortcutRowsChanged();
    void sanitizeProgramWideActionShortcuts();
    void sanitizeProgramWideActionShortcuts(QAction *action);
    ApplicationShortcutProjection shortcutProjectionForAction(
        ActionId actionId, const QAction *action) const;
    static QString actionDisplayText(const QAction *action);
    static QString shortcutDisplayText(const QAction *action);
    static QStringList shortcutKeyDisplayTexts(const QAction *action);
    QList<ShortcutHelpRow> shortcutHelpRows() const;
    void clearShortcutRouter();
    void rebuildShortcutRouter();
    void addShortcutBinding(ActionId actionId, const QList<QKeySequence> &shortcuts,
        ApplicationShortcutActivationScope activationScope,
        std::optional<ImageShortcutScope> shortcutScope = std::nullopt);
    bool handleShortcutEvent(const QKeySequence &shortcut);
    bool handleFixedShortcutEvent(const QKeySequence &shortcut);
    bool shortcutBindingEnabled(
        ActionId actionId, std::optional<ImageShortcutScope> shortcutScope) const;
    void updateShortcutEnabledStates();
    void handleShortcutActivated(ActionId actionId);

    struct ShortcutBinding {
        ActionId actionId = ActionId::ActionCount;
        ApplicationShortcutActivationScope activationScope
            = ApplicationShortcutActivationScope::ProgramWide;
        std::optional<ImageShortcutScope> shortcutScope;
        QList<QKeySequence> shortcuts;
        bool enabled = false;
    };

    ApplicationActionHost &m_host;
    const ApplicationActionRegistry &m_actionRegistry;
    ChangeCallback m_changeCallback;
    TriggerCallbacks m_triggerCallbacks;
    std::unique_ptr<ShortcutHelpModel> m_shortcutHelpModel;
    std::unique_ptr<ShortcutRouteModel> m_shortcutRouteModel;
    std::unique_ptr<QObject> m_shortcutEventFilter;
    QPointer<QObject> m_shortcutHost;
    QPointer<QWindow> m_shortcutWindow;
    std::vector<ShortcutBinding> m_shortcuts;
    std::array<QList<QKeySequence>, actionDefinitionCount> m_viewerLocalShortcuts;
    ApplicationActionStateInput m_actionStateInput;
    int m_shortcutRevision = 0;
    bool m_sanitizingShortcuts = false;
};
}

#endif
