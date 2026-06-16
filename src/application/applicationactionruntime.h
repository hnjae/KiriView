// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONRUNTIME_H
#define KIRIVIEW_APPLICATIONACTIONRUNTIME_H

#include "applicationactionhost.h"
#include "applicationactionregistry.h"
#include "applicationactionstatepolicy.h"
#include "applicationcommandrouter.h"
#include "applicationmenupresentationruntime.h"
#include "applicationshortcutpolicy.h"
#include "imageactionavailabilitypolicy.h"
#include "session/documentsessiontypes.h"

#include <KStandardActions>
#include <QAbstractListModel>
#include <QAction>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <functional>
#include <memory>

namespace kiriview::ApplicationActions {
struct ActionDefinition;
class ApplicationCommandPortSource;
class ApplicationShortcutRuntime;

struct ApplicationActionStateSnapshot {
    quint64 uiGateRevision = 0;
    kiriview::DocumentSessionActionAvailabilityFacts sessionActionAvailability;
    bool displayedMediaOpenWithAvailable = false;
    bool displayedFileDeletionAvailable = false;
    bool fileDeletionInProgress = false;
    bool activeNavigationAvailable = false;
    bool activeNavigationKnown = false;
    bool activeNavigationHasTargets = false;
    bool canOpenPreviousActiveNavigation = false;
    bool canOpenNextActiveNavigation = false;
    bool directMediaNavigationBoundaryActive = false;
    bool activeNavigationDispatchAvailable = false;
    bool imageDocumentPageNavigationActive = false;
    bool atKnownFirstActiveNavigation = false;
    bool videoMode = false;
    bool videoSeekable = false;
    qint64 videoDuration = 0;
    bool helpDialogOpen = false;
    bool textInputFocused = false;
    bool imagePannable = false;
    bool infoPanelVisible = false;
    bool thumbnailPanelVisible = false;
    bool fullscreen = false;
    bool applicationMenuShortcutEnabled = false;
    bool showMenubarActionEnabled = true;
};

class ApplicationActionRuntime final
{
public:
    struct Callbacks {
        std::function<void()> menuPresentationChanged;
        std::function<void()> shortcutRevisionChanged;
        std::function<void()> actionStateChanged;
        std::function<void(ActionId)> unsupportedVideoActionTriggered;
        std::function<void(ActionId)> unsupportedImageActionTriggered;
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
    void setActionStateSnapshot(const ApplicationActionStateSnapshot &snapshot);
    void setActionStateInput(const ApplicationActionStateInput &input);
    void setCommandPortSource(ApplicationCommandPortSource *source);
    ApplicationCommandRouterInput commandRouterInput() const;
    bool rightToLeftReadingActive() const;
    void handleActionTriggered(ActionId actionId) const;
    void handleScanForwardAction() const;
    void handleScanBackwardAction() const;
    bool executeHorizontalArrowShortcut(bool leftArrow) const;
    bool executeSinglePageArrowShortcut(bool leftArrow) const;
    bool executeVerticalPanShortcut(bool up) const;
    bool executeVideoSeekShortcut(qint64 deltaMilliseconds) const;
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
    ApplicationCommandRouterPorts commandRouterPorts() const;
    void handleActionChanged(QAction *changedAction);
    void handleActionTriggered(ActionId actionId, QAction *triggeredAction);

    ApplicationActionHost &m_host;
    ApplicationActionRegistry m_actionRegistry;
    ApplicationCommandRouter m_commandRouter;
    ApplicationMenuPresentationRuntime m_menuPresentationRuntime;
    std::unique_ptr<ApplicationShortcutRuntime> m_shortcutRuntime;
    ImageActionAvailabilityProjection m_imageActionProjection;
    ApplicationActionStateSnapshot m_actionStateSnapshot;
    ApplicationActionStateInput m_actionStateInput;
    ApplicationCommandPortSource *m_commandPortSource = nullptr;
    int m_actionStateRevision = 0;
    bool m_applyingActionState = false;
    std::function<void()> m_actionStateChanged;
};
}

#endif
