// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriviewapplication.h"
#include "kiriviewapplicationactions.h"
#include "kiriviewstate.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KirigamiActionCollection>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QFileInfo>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

namespace {
namespace Actions = KiriView::ApplicationActions;

constexpr const char *interfaceConfigGroup = "Interface";
constexpr const char *menuPresentationConfigKey = "menuPresentation";
constexpr const char *stateConfigFileName = "kiriviewstaterc";

QKeySequence shortcut(const QString &sequence)
{
    return QKeySequence::fromString(sequence, QKeySequence::PortableText);
}

QString definitionActionName(const Actions::ActionDefinition &definition)
{
    return QString::fromLatin1(definition.name);
}

bool actionDefaultShortcutsAreManagedByKiriView(const Actions::ActionDefinition &definition)
{
    return definition.kind != Actions::RegistrationKind::Inherited;
}

KConfigGroup stateInterfaceGroup()
{
    return KConfigGroup(KiriViewState::self()->config(), QLatin1String(interfaceConfigGroup));
}

void resetConfig()
{
    KiriViewState *state = KiriViewState::self();
    state->config()->deleteGroup(QStringLiteral("Interface"));
    state->config()->sync();
    state->config()->reparseConfiguration();
    state->read();

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
    void generalSettingsActionIsNotRegistered();
    void actionIdsResolveActionNamesAndShortcuts();
    void shortcutsApiReturnsCurrentShortcuts();
    void shortcutModifierPartitionsTextInputShortcuts();
    void showMenubarActionHasNoConfigurableShortcuts();
    void menuPresentationDefaultsToHamburgerMenu();
    void invalidStoredMenuPresentationFallsBackToHamburgerMenu();
    void menuPresentationPersists();
    void menuPresentationStateUsesGenericStateLocation();
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

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString actionName = definitionActionName(definition);
        QVERIFY2(application.action(actionName) != nullptr,
            qPrintable(QStringLiteral("Missing action %1").arg(actionName)));
        if (actionDefaultShortcutsAreManagedByKiriView(definition)) {
            expectDefaultShortcuts(
                application, actionName, Actions::defaultShortcuts(definition.defaultShortcuts));
        }
    }
}

void TestKiriViewApplication::generalSettingsActionIsNotRegistered()
{
    KiriViewApplication application;

    QCOMPARE(application.action(QStringLiteral("options_configure")), nullptr);
    QCOMPARE(application.shortcuts(QStringLiteral("options_configure")), QList<QKeySequence>());
}

void TestKiriViewApplication::actionIdsResolveActionNamesAndShortcuts()
{
    KiriViewApplication application;

    for (const Actions::ActionDefinition &definition : Actions::definitions()) {
        const QString actionName = definitionActionName(definition);
        QCOMPARE(application.actionName(definition.actionId), actionName);
        QCOMPARE(application.actionForId(definition.actionId), application.action(actionName));
        QCOMPARE(
            application.shortcutsForId(definition.actionId), application.shortcuts(actionName));
        QCOMPARE(application.shortcutsWithCommandModifierForId(definition.actionId),
            application.shortcutsWithCommandModifier(actionName));
        QCOMPARE(application.shortcutsWithoutCommandModifierForId(definition.actionId),
            application.shortcutsWithoutCommandModifier(actionName));
        QCOMPARE(application.shortcutTextForId(definition.actionId),
            application.shortcutText(actionName));
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
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_two_page_mode")),
        QList<QKeySequence>({ shortcut(QStringLiteral("D")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("view_toggle_right_to_left_reading")),
        QList<QKeySequence>({ shortcut(QStringLiteral("B")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("go_previous_single_page")),
        QList<QKeySequence>({ shortcut(QStringLiteral("P")) }));
    QCOMPARE(application.shortcuts(QStringLiteral("go_next_single_page")),
        QList<QKeySequence>({ shortcut(QStringLiteral("N")) }));
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

void TestKiriViewApplication::showMenubarActionHasNoConfigurableShortcuts()
{
    KiriViewApplication application;

    QAction *showMenubarAction = application.action(QStringLiteral("options_show_menubar"));
    QVERIFY(showMenubarAction != nullptr);
    QCOMPARE(KirigamiActionCollection::defaultShortcuts(showMenubarAction), QList<QKeySequence>());
    QCOMPARE(showMenubarAction->shortcuts(), QList<QKeySequence>());
    QVERIFY(!KirigamiActionCollection::isShortcutsConfigurable(showMenubarAction));
}

void TestKiriViewApplication::menuPresentationDefaultsToHamburgerMenu()
{
    KiriViewApplication application;

    QCOMPARE(KiriViewState::defaultMenuPresentationValue(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::HamburgerMenu));
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QCOMPARE(KiriViewState::menuPresentation(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::HamburgerMenu));
    QVERIFY(!stateInterfaceGroup().hasKey(QLatin1String(menuPresentationConfigKey)));
}

void TestKiriViewApplication::invalidStoredMenuPresentationFallsBackToHamburgerMenu()
{
    KiriViewState::setMenuPresentation(99);

    KiriViewApplication application;

    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);
}

void TestKiriViewApplication::menuPresentationPersists()
{
    KiriViewApplication application;
    QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

    application.setMenuPresentation(KiriViewApplication::MenuBar);

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
    QCOMPARE(KiriViewState::menuPresentation(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::MenuBar));
    QCOMPARE(stateInterfaceGroup().readEntry(QLatin1String(menuPresentationConfigKey), QString()),
        QStringLiteral("MenuBar"));

    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::HamburgerMenu);
    QCOMPARE(application.menuPresentation(), KiriViewApplication::HamburgerMenu);

    KiriViewState::self()->read();
    QCOMPARE(application.menuPresentation(), KiriViewApplication::MenuBar);
}

void TestKiriViewApplication::menuPresentationStateUsesGenericStateLocation()
{
    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::MenuBar);
    KiriViewState::self()->save();

    const QString statePath = QStandardPaths::locate(
        QStandardPaths::GenericStateLocation, QLatin1String(stateConfigFileName));
    QVERIFY(!statePath.isEmpty());
    QCOMPARE(QFileInfo(statePath).absolutePath(),
        QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation));
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
