// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationmenupresentationruntime.h"
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
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime(application);

    QCOMPARE(runtime.menuPresentation(), KiriViewApplication::HamburgerMenu);
}

void TestApplicationMenuPresentationRuntime::setMenuPresentationPersistsAndSignalsChanges()
{
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime(application);
    QSignalSpy changedSpy(&application, &KiriViewApplication::menuPresentationChanged);

    runtime.setMenuPresentation(KiriViewApplication::MenuBar);

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(runtime.menuPresentation(), KiriViewApplication::MenuBar);
    QCOMPARE(KiriViewState::menuPresentation(),
        static_cast<int>(KiriViewState::EnumMenuPresentation::MenuBar));
    QCOMPARE(stateInterfaceGroup().readEntry(QLatin1String(menuPresentationConfigKey), QString()),
        QStringLiteral("MenuBar"));

    runtime.setMenuPresentation(KiriViewApplication::MenuBar);
    QCOMPARE(changedSpy.count(), 1);
}

void TestApplicationMenuPresentationRuntime::showMenuBarActionMirrorsAndUpdatesPresentation()
{
    KiriViewState::setMenuPresentation(KiriViewState::EnumMenuPresentation::MenuBar);
    KiriViewApplication application;
    KiriView::ApplicationActions::ApplicationMenuPresentationRuntime runtime(application);
    QAction action;

    runtime.bindShowMenuBarAction(&action);

    QVERIFY(action.isCheckable());
    QVERIFY(action.isChecked());

    action.trigger();
    QCOMPARE(runtime.menuPresentation(), KiriViewApplication::HamburgerMenu);
    QVERIFY(!action.isChecked());

    action.trigger();
    QCOMPARE(runtime.menuPresentation(), KiriViewApplication::MenuBar);
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
