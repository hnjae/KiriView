// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"
#include "kiriviewsettings.h"

#include <KConfig>
#include <KSharedConfig>
#include <KirigamiActionCollection>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

namespace {
QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

void resetConfig()
{
    KiriViewSettings *settings = KiriViewSettings::self();
    settings->config()->deleteGroup(QStringLiteral("Interface"));
    settings->config()->sync();
    settings->config()->reparseConfiguration();
    settings->read();

    KSharedConfig::Ptr appConfig = KSharedConfig::openConfig();
    appConfig->deleteGroup(QStringLiteral("Shortcuts"));
    appConfig->sync();
    appConfig->reparseConfiguration();
}

void expectDefaultShortcuts(KiriViewApplication &application, const QString &actionName,
    const QList<QKeySequence> &shortcuts)
{
    QAction *action = application.action(actionName);
    QVERIFY2(action != nullptr, qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
    QCOMPARE(KirigamiActionCollection::defaultShortcuts(action), shortcuts);
    QCOMPARE(action->shortcuts(), shortcuts);
}
}

class TestKiriViewApplication : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void actionsAreRegisteredWithDefaultShortcuts();
    void actionIdsResolveActionNamesAndShortcuts();
    void shortcutsApiReturnsCurrentShortcuts();
    void shortcutModifierPartitionsTextInputShortcuts();
    void menuPresentationDefaultsToHamburgerMenu();
    void menuPresentationPersists();
    void showMenubarActionTogglesMenuPresentation();
};

void TestKiriViewApplication::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();
}

void TestKiriViewApplication::init() { resetConfig(); }

void TestKiriViewApplication::cleanup() { resetConfig(); }

void TestKiriViewApplication::actionsAreRegisteredWithDefaultShortcuts()
{
    KiriViewApplication application;

    const QStringList registeredActionNames = {
        QStringLiteral("file_open"),
        QStringLiteral("go_previous_archive"),
        QStringLiteral("go_next_archive"),
        QStringLiteral("go_previous_image"),
        QStringLiteral("go_next_image"),
        QStringLiteral("go_first_image"),
        QStringLiteral("go_last_image"),
        QStringLiteral("view_zoom_in"),
        QStringLiteral("view_zoom_out"),
        QStringLiteral("view_fit"),
        QStringLiteral("view_fit_height"),
        QStringLiteral("view_fit_width"),
        QStringLiteral("view_actual_size"),
        QStringLiteral("view_pan_left"),
        QStringLiteral("view_pan_right"),
        QStringLiteral("view_pan_up"),
        QStringLiteral("view_pan_down"),
        QStringLiteral("view_pan_top_left"),
        QStringLiteral("view_pan_bottom_right"),
        QStringLiteral("view_scan_forward"),
        QStringLiteral("view_scan_backward"),
        QStringLiteral("window_fullscreen"),
        QStringLiteral("help_shortcuts"),
        QStringLiteral("options_show_menubar"),
        QStringLiteral("options_configure_keybinding"),
        QStringLiteral("file_quit"),
    };
    for (const QString &actionName : registeredActionNames) {
        QVERIFY2(application.action(actionName) != nullptr,
            qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
    }

    QObject configurationView;
    application.setConfigurationView(&configurationView);
    QVERIFY(application.action(QStringLiteral("options_configure")) != nullptr);

    expectDefaultShortcuts(
        application, QStringLiteral("file_open"), QKeySequence::keyBindings(QKeySequence::Open));
    expectDefaultShortcuts(
        application, QStringLiteral("go_previous_archive"), { shortcut(QStringLiteral("[")) });
    expectDefaultShortcuts(
        application, QStringLiteral("go_next_archive"), { shortcut(QStringLiteral("]")) });
    expectDefaultShortcuts(application, QStringLiteral("go_previous_image"),
        QKeySequence::keyBindings(QKeySequence::MoveToPreviousPage));
    expectDefaultShortcuts(application, QStringLiteral("go_next_image"),
        QKeySequence::keyBindings(QKeySequence::MoveToNextPage));
    expectDefaultShortcuts(application, QStringLiteral("go_first_image"),
        { shortcut(QStringLiteral("Ctrl+Home")), shortcut(QStringLiteral("Home")) });
    expectDefaultShortcuts(application, QStringLiteral("go_last_image"),
        { shortcut(QStringLiteral("Ctrl+End")), shortcut(QStringLiteral("End")) });
    expectDefaultShortcuts(application, QStringLiteral("view_zoom_in"),
        { shortcut(QStringLiteral("Ctrl+=")), shortcut(QStringLiteral("Ctrl++")),
            shortcut(QStringLiteral("=")), shortcut(QStringLiteral("+")) });
    expectDefaultShortcuts(application, QStringLiteral("view_zoom_out"),
        { shortcut(QStringLiteral("-")), shortcut(QStringLiteral("Ctrl+-")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_fit"), { shortcut(QStringLiteral("1")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_fit_height"), { shortcut(QStringLiteral("2")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_fit_width"), { shortcut(QStringLiteral("3")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_actual_size"), { shortcut(QStringLiteral("0")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_left"), { shortcut(QStringLiteral("Left")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_right"), { shortcut(QStringLiteral("Right")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_up"), { shortcut(QStringLiteral("Up")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_down"), { shortcut(QStringLiteral("Down")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_top_left"), { shortcut(QStringLiteral("<")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_pan_bottom_right"), { shortcut(QStringLiteral(">")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_scan_forward"), { shortcut(QStringLiteral(".")) });
    expectDefaultShortcuts(
        application, QStringLiteral("view_scan_backward"), { shortcut(QStringLiteral(",")) });
    expectDefaultShortcuts(application, QStringLiteral("window_fullscreen"),
        { shortcut(QStringLiteral("F")), shortcut(QStringLiteral("F11")) });
    expectDefaultShortcuts(application, QStringLiteral("help_shortcuts"),
        { shortcut(QStringLiteral("?")), shortcut(QStringLiteral("F1")) });
    expectDefaultShortcuts(application, QStringLiteral("file_quit"),
        { shortcut(QStringLiteral("Q")), shortcut(QStringLiteral("Ctrl+Q")) });
}

void TestKiriViewApplication::actionIdsResolveActionNamesAndShortcuts()
{
    struct ExpectedActionId {
        KiriViewApplication::ActionId actionId;
        const char *actionName;
    };

    const ExpectedActionId expectedActionIds[] = {
        { KiriViewApplication::FileOpenAction, "file_open" },
        { KiriViewApplication::GoPreviousArchiveAction, "go_previous_archive" },
        { KiriViewApplication::GoNextArchiveAction, "go_next_archive" },
        { KiriViewApplication::GoPreviousImageAction, "go_previous_image" },
        { KiriViewApplication::GoNextImageAction, "go_next_image" },
        { KiriViewApplication::GoFirstImageAction, "go_first_image" },
        { KiriViewApplication::GoLastImageAction, "go_last_image" },
        { KiriViewApplication::ViewZoomInAction, "view_zoom_in" },
        { KiriViewApplication::ViewZoomOutAction, "view_zoom_out" },
        { KiriViewApplication::ViewFitAction, "view_fit" },
        { KiriViewApplication::ViewFitHeightAction, "view_fit_height" },
        { KiriViewApplication::ViewFitWidthAction, "view_fit_width" },
        { KiriViewApplication::ViewActualSizeAction, "view_actual_size" },
        { KiriViewApplication::ViewPanLeftAction, "view_pan_left" },
        { KiriViewApplication::ViewPanRightAction, "view_pan_right" },
        { KiriViewApplication::ViewPanUpAction, "view_pan_up" },
        { KiriViewApplication::ViewPanDownAction, "view_pan_down" },
        { KiriViewApplication::ViewPanTopLeftAction, "view_pan_top_left" },
        { KiriViewApplication::ViewPanBottomRightAction, "view_pan_bottom_right" },
        { KiriViewApplication::ViewScanForwardAction, "view_scan_forward" },
        { KiriViewApplication::ViewScanBackwardAction, "view_scan_backward" },
        { KiriViewApplication::WindowFullscreenAction, "window_fullscreen" },
        { KiriViewApplication::HelpShortcutsAction, "help_shortcuts" },
        { KiriViewApplication::OptionsConfigureAction, "options_configure" },
        { KiriViewApplication::OptionsConfigureKeybindingAction, "options_configure_keybinding" },
        { KiriViewApplication::OptionsShowMenubarAction, "options_show_menubar" },
        { KiriViewApplication::FileQuitAction, "file_quit" },
    };

    KiriViewApplication application;
    QObject configurationView;
    application.setConfigurationView(&configurationView);

    for (const ExpectedActionId &expected : expectedActionIds) {
        const QString actionName = QString::fromLatin1(expected.actionName);
        QCOMPARE(application.actionName(expected.actionId), actionName);
        QCOMPARE(application.actionForId(expected.actionId), application.action(actionName));
        QCOMPARE(application.shortcutsForId(expected.actionId), application.shortcuts(actionName));
        QCOMPARE(application.shortcutsWithCommandModifierForId(expected.actionId),
            application.shortcutsWithCommandModifier(actionName));
        QCOMPARE(application.shortcutsWithoutCommandModifierForId(expected.actionId),
            application.shortcutsWithoutCommandModifier(actionName));
        QCOMPARE(
            application.shortcutTextForId(expected.actionId), application.shortcutText(actionName));
    }

    const auto invalidActionId = static_cast<KiriViewApplication::ActionId>(-1);
    QVERIFY(application.actionName(invalidActionId).isEmpty());
    QCOMPARE(application.actionForId(invalidActionId), nullptr);
    QCOMPARE(application.shortcutsForId(invalidActionId), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(invalidActionId).isEmpty());

    const auto outOfRangeActionId = static_cast<KiriViewApplication::ActionId>(999);
    QVERIFY(application.actionName(outOfRangeActionId).isEmpty());
    QCOMPARE(application.actionForId(outOfRangeActionId), nullptr);
    QCOMPARE(application.shortcutsForId(outOfRangeActionId), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(outOfRangeActionId).isEmpty());

    QVERIFY(application.actionName(KiriViewApplication::ActionCount).isEmpty());
    QCOMPARE(application.actionForId(KiriViewApplication::ActionCount), nullptr);
    QCOMPARE(application.shortcutsForId(KiriViewApplication::ActionCount), QList<QKeySequence>());
    QVERIFY(application.shortcutTextForId(KiriViewApplication::ActionCount).isEmpty());
}

void TestKiriViewApplication::shortcutsApiReturnsCurrentShortcuts()
{
    KiriViewApplication application;

    QCOMPARE(application.shortcuts(QStringLiteral("view_zoom_in")),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Ctrl+=")), shortcut(QStringLiteral("Ctrl++")),
                shortcut(QStringLiteral("=")), shortcut(QStringLiteral("+")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("missing_action")), QList<QKeySequence>());

    QAction *openAction = application.action(QStringLiteral("file_open"));
    QVERIFY(openAction != nullptr);
    const QList<QKeySequence> customShortcuts = {
        shortcut(QStringLiteral("Alt+O")),
        shortcut(QStringLiteral("Ctrl+Shift+O")),
    };

    openAction->setShortcuts(customShortcuts);

    QCOMPARE(application.shortcuts(QStringLiteral("file_open")), customShortcuts);
}

void TestKiriViewApplication::shortcutModifierPartitionsTextInputShortcuts()
{
    KiriViewApplication application;

    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Q")) }));
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Ctrl+Q")) }));
    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("missing_action")),
        QList<QKeySequence>());
    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("missing_action")),
        QList<QKeySequence>());

    QAction *quitAction = application.action(QStringLiteral("file_quit"));
    QVERIFY(quitAction != nullptr);
    quitAction->setShortcuts({ shortcut(QStringLiteral("Alt+Q")),
        shortcut(QStringLiteral("Shift+Q")), shortcut(QStringLiteral("Meta+Q")),
        shortcut(QStringLiteral("Ctrl+Shift+Q")), shortcut(QStringLiteral("Q")) });

    QCOMPARE(application.shortcutsWithCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>({ shortcut(QStringLiteral("Alt+Q")), shortcut(QStringLiteral("Meta+Q")),
            shortcut(QStringLiteral("Ctrl+Shift+Q")) }));
    QCOMPARE(application.shortcutsWithoutCommandModifier(QStringLiteral("file_quit")),
        QList<QKeySequence>(
            { shortcut(QStringLiteral("Shift+Q")), shortcut(QStringLiteral("Q")) }));
}

void TestKiriViewApplication::menuPresentationDefaultsToHamburgerMenu()
{
    KiriViewApplication application;

    QCOMPARE(KiriViewSettings::defaultMenuPresentationValue(),
        static_cast<int>(KiriViewSettings::EnumMenuPresentation::HamburgerMenu));
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QCOMPARE(KiriViewSettings::menuPresentation(),
        static_cast<int>(KiriViewSettings::EnumMenuPresentation::HamburgerMenu));
}

void TestKiriViewApplication::menuPresentationPersists()
{
    KiriViewApplication application;
    QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

    application.setMenuPresentation(KiriViewApplication::MenuBar);

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
    QCOMPARE(KiriViewSettings::menuPresentation(),
        static_cast<int>(KiriViewSettings::EnumMenuPresentation::MenuBar));

    KiriViewSettings::setMenuPresentation(KiriViewSettings::EnumMenuPresentation::HamburgerMenu);
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);

    KiriViewSettings::self()->read();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
}

void TestKiriViewApplication::showMenubarActionTogglesMenuPresentation()
{
    KiriViewApplication application;

    QAction *showMenubarAction = application.action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QVERIFY(showMenubarAction->isCheckable());
    QVERIFY(!showMenubarAction->isChecked());
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);

    showMenubarAction->trigger();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
    QVERIFY(showMenubarAction->isChecked());

    showMenubarAction->trigger();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QVERIFY(!showMenubarAction->isChecked());
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestKiriViewApplication test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_kiriviewapplication.moc"
