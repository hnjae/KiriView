// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIVIEWAPPLICATION_H
#define KIRIVIEW_KIRIVIEWAPPLICATION_H

#include <AbstractKirigamiApplication>
#include <KStandardActions>

#include <QAbstractListModel>
#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

class KiriViewApplication : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(MenuPresentation menuPresentation READ menuPresentation WRITE setMenuPresentation
            NOTIFY menuPresentationChanged)
    Q_PROPERTY(int shortcutRevision READ shortcutRevision NOTIFY shortcutRevisionChanged)
    Q_PROPERTY(QAbstractListModel *shortcutHelpModel READ shortcutHelpModel CONSTANT)

public:
    enum MenuPresentation {
        HamburgerMenu = 0,
        MenuBar = 1,
    };
    Q_ENUM(MenuPresentation)

    enum ActionId {
        FileOpenAction = 0,
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
        ViewPanTopLeftAction,
        ViewPanBottomRightAction,
        ViewScanForwardAction,
        ViewScanBackwardAction,
        GoPreviousSinglePageAction,
        GoNextSinglePageAction,
        WindowFullscreenAction,
        HelpShortcutsAction,
        OptionsConfigureKeybindingAction,
        OptionsShowMenubarAction,
        FileQuitAction,
        ActionCount,
    };
    Q_ENUM(ActionId)

    explicit KiriViewApplication(QObject *parent = nullptr);
    ~KiriViewApplication() override;

    MenuPresentation menuPresentation() const;
    void setMenuPresentation(MenuPresentation presentation);
    int shortcutRevision() const;
    QAbstractListModel *shortcutHelpModel() const;

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

Q_SIGNALS:
    void menuPresentationChanged();
    void shortcutRevisionChanged();

protected:
    void setupActions() override;

private:
    QAction *addRegisteredAction(const QString &name, const QString &text, const QString &iconName,
        const QList<QKeySequence> &defaultShortcuts = {});
    QAction *addStandardAction(KStandardActions::StandardAction actionType, const QString &name,
        const QString &text, const QList<QKeySequence> &defaultShortcuts);
    QAction *finishRegisteredAction(QAction *registeredAction, const QString &text,
        const QList<QKeySequence> &defaultShortcuts);
    void handleActionChanged();
    void sanitizeActionShortcuts();
    void sanitizeActionShortcuts(QAction *action);
    void updateShowMenuBarAction();

    QAction *m_showMenuBarAction = nullptr;
    QAbstractListModel *m_shortcutHelpModel = nullptr;
    int m_shortcutRevision = 0;
    bool m_sanitizingShortcuts = false;
};

#endif
