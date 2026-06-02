// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIEWAPPLICATION_H
#define KIRIVIEW_KIRIVIEWAPPLICATION_H

#include "application/applicationtypes.h"

#include <AbstractKirigamiApplication>
#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace KiriView::ApplicationActions {
class KiriViewApplicationActionHost;
class ApplicationActionRuntime;
class ApplicationShortcutRuntime;
}

class KiriViewApplication : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(MenuPresentation menuPresentation READ menuPresentation WRITE setMenuPresentation
            NOTIFY menuPresentationChanged)
    Q_PROPERTY(int shortcutRevision READ shortcutRevision NOTIFY shortcutRevisionChanged)
    Q_PROPERTY(int actionStateRevision READ actionStateRevision NOTIFY actionStateRevisionChanged)
    Q_PROPERTY(QAbstractListModel *shortcutHelpModel READ shortcutHelpModel CONSTANT)
    Q_PROPERTY(QAbstractListModel *shortcutRouteModel READ shortcutRouteModel CONSTANT)

public:
    enum MenuPresentation {
        HamburgerMenu = 0,
        MenuBar = 1,
    };
    Q_ENUM(MenuPresentation)

    enum ActionId {
        FileOpenAction = 0,
        FileOpenWithAction,
        FileMoveToTrashAction,
        FileDeleteAction,
        GoPreviousArchiveAction,
        GoNextArchiveAction,
        GoPreviousImageAction,
        GoNextImageAction,
        GoFirstImageAction,
        GoLastImageAction,
        ViewZoomInAction,
        ViewZoomOutAction,
        ViewFitAction,
        ViewFitHeightAction,
        ViewFitWidthAction,
        ViewActualSizeAction,
        ViewRotateClockwiseAction,
        ViewRotateCounterclockwiseAction,
        ViewToggleTwoPageModeAction,
        ViewToggleRightToLeftReadingAction,
        ViewToggleInfoPanelAction,
        ViewToggleThumbnailPanelAction,
        ViewPanTopLeftAction,
        ViewPanBottomRightAction,
        ViewScanForwardAction,
        ViewScanBackwardAction,
        WindowFullscreenAction,
        HelpShortcutsAction,
        OptionsConfigureKeybindingAction,
        OptionsShowMenubarAction,
        OpenApplicationMenuAction,
        FileQuitAction,
        ActionCount,
    };
    Q_ENUM(ActionId)

    explicit KiriViewApplication(QObject *parent = nullptr);
    ~KiriViewApplication() override;

    MenuPresentation menuPresentation() const;
    void setMenuPresentation(MenuPresentation presentation);
    int shortcutRevision() const;
    int actionStateRevision() const;
    QAbstractListModel *shortcutHelpModel() const;
    QAbstractListModel *shortcutRouteModel() const;

    static KiriView::ApplicationActions::MenuPresentation domainMenuPresentation(
        KiriViewApplication::MenuPresentation presentation);
    static KiriViewApplication::MenuPresentation facadeMenuPresentation(
        KiriView::ApplicationActions::MenuPresentation presentation);
    static KiriView::ApplicationActions::ActionId domainActionId(
        KiriViewApplication::ActionId actionId);
    static KiriViewApplication::ActionId facadeActionId(
        KiriView::ApplicationActions::ActionId actionId);

    Q_INVOKABLE QAction *action(const QString &actionName);
    Q_INVOKABLE QAction *actionForId(KiriViewApplication::ActionId actionId);
    Q_INVOKABLE QString actionName(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QList<QKeySequence> shortcuts(const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> shortcutsForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QList<QKeySequence> shortcutsWithCommandModifier(const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> shortcutsWithCommandModifierForId(
        KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QList<QKeySequence> shortcutsWithoutCommandModifier(
        const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> shortcutsWithoutCommandModifierForId(
        KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QList<QKeySequence> shortcutAliases(const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> shortcutAliasesForId(
        KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QString shortcutText(const QString &actionName) const;
    Q_INVOKABLE QString shortcutTextForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QKeySequence menuShortcut(const QString &actionName) const;
    Q_INVOKABLE QKeySequence menuShortcutForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QString menuShortcutText(const QString &actionName) const;
    Q_INVOKABLE QString menuShortcutTextForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE bool actionPlacementEnabled(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QString actionMenuTextForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QString actionToolbarTextForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QString actionToolbarTooltipTextForId(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE void updateActionState(bool helpActionsEnabled, bool readyActionsEnabled,
        bool rotateActionsEnabled, bool twoPageModeActionsEnabled,
        bool rightToLeftReadingActionsEnabled, bool containerNavigationActionsEnabled,
        bool displayedMediaOpenWithAvailable, bool displayedFileDeletionAvailable,
        bool fileDeletionInProgress, bool activeNavigationAvailable, bool activeNavigationKnown,
        bool activeNavigationHasTargets, bool canOpenPreviousActiveNavigation,
        bool canOpenNextActiveNavigation, bool fitModeSelected, bool fitHeightModeSelected,
        bool fitWidthModeSelected, bool twoPageModeActive, bool rightToLeftReadingActive,
        bool infoPanelVisible, bool thumbnailPanelVisible, bool fullscreen,
        bool applicationMenuShortcutEnabled, bool showMenubarActionEnabled,
        bool directMediaNavigationBoundaryActive, bool viewerShortcutsEnabled,
        bool readyShortcutsEnabled, bool readyViewerShortcutsEnabled,
        bool twoPageViewerShortcutsEnabled, bool rightToLeftReadingShortcutsEnabled,
        bool rightToLeftReadingViewerShortcutsEnabled, bool rotateShortcutsEnabled,
        bool rotateViewerShortcutsEnabled, bool pannableShortcutsEnabled,
        bool pannableViewerShortcutsEnabled, bool containerViewerShortcutsEnabled,
        bool activeNavigationActionsAvailable, bool videoMode, bool videoFileDeletionInProgress);
    Q_INVOKABLE void setShortcutHost(QObject *host);
    Q_INVOKABLE bool videoActionUnsupported(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE bool mediaHorizontalArrowShortcutsEnabled(bool videoMode,
        bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
        bool videoDirectMediaNavigationActive, bool videoFileDeletionInProgress) const;

Q_SIGNALS:
    void menuPresentationChanged();
    void shortcutRevisionChanged();
    void actionStateRevisionChanged();
    void actionTriggered(KiriViewApplication::ActionId actionId);
    void unsupportedVideoActionTriggered(KiriViewApplication::ActionId actionId);

protected:
    void setupActions() override;

private:
    friend class KiriView::ApplicationActions::KiriViewApplicationActionHost;

    KirigamiActionCollection *applicationMainActionCollection();
    QAction *inheritedApplicationAction(const QString &actionName);
    void readApplicationActionSettings();

    std::unique_ptr<KiriView::ApplicationActions::KiriViewApplicationActionHost> m_actionHost;
    std::unique_ptr<KiriView::ApplicationActions::ApplicationActionRuntime> m_actionRuntime;
};

#endif
