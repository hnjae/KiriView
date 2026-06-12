// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIEWAPPLICATION_H
#define KIRIVIEW_KIRIVIEWAPPLICATION_H

#include "application/applicationactionstatepolicy.h"
#include "application/applicationcommandrouter.h"
#include "application/applicationtypes.h"
#include "application/imageactionavailabilitypolicy.h"

#include <AbstractKirigamiApplication>
#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <vector>

namespace KiriView::ApplicationActions {
class KiriViewApplicationActionHost;
class ApplicationActionRuntime;
class ApplicationShortcutRuntime;
}

class KiriDocumentSession;
class KiriImageDocument;

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
        ViewZoom50PercentAction,
        ViewZoom100PercentAction,
        ViewZoom200PercentAction,
        ViewFitAction,
        ViewFitHeightAction,
        ViewFitWidthAction,
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
    Q_INVOKABLE QList<QKeySequence> programWideShortcuts(const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> programWideShortcutsForId(
        KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE QList<QKeySequence> viewerLocalShortcuts(const QString &actionName) const;
    Q_INVOKABLE QList<QKeySequence> viewerLocalShortcutsForId(
        KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE bool setViewerLocalShortcuts(
        const QString &actionName, const QList<QKeySequence> &shortcuts);
    Q_INVOKABLE bool setViewerLocalShortcutsForId(
        KiriViewApplication::ActionId actionId, const QList<QKeySequence> &shortcuts);
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
    Q_INVOKABLE void setDocumentSession(QObject *session);
    Q_INVOKABLE void updateActionUiGateSnapshot(bool helpDialogOpen, bool textInputFocused,
        bool infoPanelVisible, bool thumbnailPanelVisible, bool fullscreen,
        bool applicationMenuShortcutEnabled, bool showMenubarActionEnabled);
    Q_INVOKABLE void setShortcutHost(QObject *host);
    Q_INVOKABLE bool videoActionUnsupported(KiriViewApplication::ActionId actionId) const;
    Q_INVOKABLE bool mediaHorizontalArrowShortcutsEnabled(bool videoMode,
        bool imageReadyViewerShortcutsEnabled, bool videoViewerShortcutsEnabled,
        bool videoDirectMediaNavigationActive, bool videoFileDeletionInProgress) const;

Q_SIGNALS:
    void menuPresentationChanged();
    void shortcutRevisionChanged();
    void actionStateRevisionChanged();
    void openDialogRequested();
    void openApplicationMenuRequested();
    void shortcutHelpRequested();
    void toggleFullScreenRequested();
    void toggleInfoPanelRequested();
    void toggleThumbnailPanelRequested();
    void imageBoundaryReached(const QString &message);
    void unsupportedVideoActionTriggered(KiriViewApplication::ActionId actionId);

protected:
    void setupActions() override;

private:
    friend class KiriView::ApplicationActions::KiriViewApplicationActionHost;

    struct ActionUiGateSnapshot {
        bool helpDialogOpen = false;
        bool textInputFocused = false;
        bool infoPanelVisible = false;
        bool thumbnailPanelVisible = false;
        bool fullscreen = false;
        bool applicationMenuShortcutEnabled = false;
        bool showMenubarActionEnabled = true;
    };

    KirigamiActionCollection *applicationMainActionCollection();
    QAction *inheritedApplicationAction(const QString &actionName);
    void readApplicationActionSettings();
    void rebuildActionState();
    void connectActionStateSources();
    void disconnectActionStateSources();
    KiriImageDocument *imageDocument() const;
    bool imageMode() const;
    bool videoMode() const;
    bool sharedImagePannable() const;
    ImageActionAvailabilityInput imageActionAvailabilityInput() const;
    KiriView::ApplicationActions::ApplicationActionStateInput actionStateInput() const;
    KiriView::ApplicationActions::ApplicationCommandRouterInput commandRouterInput() const;
    KiriView::ApplicationActions::ApplicationCommandRouterPorts commandRouterPorts();
    void handleRuntimeActionTriggered(KiriView::ApplicationActions::ActionId actionId);
    void moveDisplayedFileToTrash();
    void deleteDisplayedFilePermanently();
    void requestImageFitMode();
    void requestImageFitHeightMode();
    void requestImageFitWidthMode();
    void emitBoundaryText(const QString &message);
    void requestPreviousActiveNavigationWithBoundary();
    void requestNextActiveNavigationWithBoundary();
    void handleScanForwardAction();
    void handleScanBackwardAction();
    void updateActionUiGateSnapshot(ActionUiGateSnapshot snapshot);
    void applyActionUiGateSnapshot(const ActionUiGateSnapshot &snapshot);
    bool executeHorizontalArrowShortcut(bool leftArrow);
    bool executeSinglePageArrowShortcut(bool leftArrow);
    bool executeVerticalPanShortcut(bool up);
    bool executeVideoSeekShortcut(qint64 deltaMilliseconds);

    std::unique_ptr<KiriView::ApplicationActions::KiriViewApplicationActionHost> m_actionHost;
    std::unique_ptr<KiriView::ApplicationActions::ApplicationActionRuntime> m_actionRuntime;
    QPointer<KiriDocumentSession> m_documentSession;
    std::vector<QMetaObject::Connection> m_actionStateConnections;
    KiriView::ApplicationActions::ApplicationCommandRouter m_commandRouter;
    ImageActionAvailabilityProjection m_imageActionProjection;
    KiriView::ApplicationActions::ApplicationActionStateInput m_actionStateInput;
    quint64 m_actionUiGateRevision = 0;
    bool m_helpDialogOpen = false;
    bool m_textInputFocused = false;
    bool m_infoPanelVisible = false;
    bool m_thumbnailPanelVisible = false;
    bool m_fullscreen = false;
    bool m_applicationMenuShortcutEnabled = false;
    bool m_showMenubarActionEnabled = true;
};

#endif
