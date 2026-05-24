// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationmenupresentationruntime.h"
#include "facade/kiriviewapplication.h"
#include "kiriviewstate.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

namespace {
using MenuPresentation = KiriView::ApplicationActions::MenuPresentation;
constexpr const char *interfaceConfigGroup = "Interface";
constexpr const char *menuPresentationConfigKey = "menuPresentation";

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

KiriView::ApplicationActions::ApplicationMenuPresentationRuntime createRuntime(
    KiriViewApplication &application)
{
    return KiriView::ApplicationActions::ApplicationMenuPresentationRuntime(
        application, [&application]() { Q_EMIT application.menuPresentationChanged(); });
}
}

class TestApplicationMenuPresentationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void invalidStoredValueFallsBackToHamburgerMenu();
    void setMenuPresentationPersistsAndSignalsChanges();
    void syncFromSettingsUpdatesRuntimeStateAndAction();
    void showMenuBarActionMirrorsAndUpdatesPresentation();
};

void TestApplicationMenuPresentationRuntime::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    resetConfig();
}

void TestApplicationMenuPresentationRuntime::init() { resetConfig(); }

void TestApplicationMenuPresentationRuntime::cleanup() { resetConfig(); }

void TestApplicationMenuPresentationRuntime::invalidStoredValueFallsBackToHamburgerMenu()
{
    KiriViewState::setMenuPresentation(99);
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime
        = createRuntime(application);

    QCOMPARE(runtime.menuPresentation(), MenuPresentation::HamburgerMenu);
}

void TestApplicationMenuPresentationRuntime::setMenuPresentationPersistsAndSignalsChanges()
{
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime
        = createRuntime(application);
    QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

    runtime.setMenuPresentation(MenuPresentation::MenuBar);

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(runtime.menuPresentation(), MenuPresentation::MenuBar);
    QCOMPARE(KiriViewState::menuPresentation(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::MenuBar));
    QCOMPARE(stateInterfaceGroup().readEntry(QLatin1String(menuPresentationConfigKey), QString()),
        QStringLiteral("MenuBar"));

    runtime.setMenuPresentation(MenuPresentation::MenuBar);
    QCOMPARE(changedSpy.count(), 1);
}

void TestApplicationMenuPresentationRuntime::syncFromSettingsUpdatesRuntimeStateAndAction()
{
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime
        = createRuntime(application);
    QAction action;
    QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

    runtime.bindShowMenuBarAction(&action);
    QVERIFY(!action.isChecked());

    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::MenuBar);
    runtime.syncFromSettings();

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(runtime.menuPresentation(), MenuPresentation::MenuBar);
    QVERIFY(action.isChecked());

    runtime.syncFromSettings();
    QCOMPARE(changedSpy.count(), 1);
    QVERIFY(action.isChecked());
}

void TestApplicationMenuPresentationRuntime::showMenuBarActionMirrorsAndUpdatesPresentation()
{
    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::MenuBar);
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime
        = createRuntime(application);
    QAction action;

    runtime.bindShowMenuBarAction(&action);

    QVERIFY(action.isCheckable());
    QVERIFY(action.isChecked());

    action.trigger();
    QCOMPARE(runtime.menuPresentation(), MenuPresentation::HamburgerMenu);
    QVERIFY(!action.isChecked());

    action.trigger();
    QCOMPARE(runtime.menuPresentation(), MenuPresentation::MenuBar);
    QVERIFY(action.isChecked());
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestApplicationMenuPresentationRuntime test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_applicationmenupresentationruntime.moc"
