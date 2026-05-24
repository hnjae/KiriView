// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationmenupresentationstate.h"

#include <QObject>
#include <QTest>

namespace {
using MenuPresentation = KiriView::ApplicationActions::MenuPresentation;
}

class TestApplicationMenuPresentationState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void invalidStoredValuesNormalizeToHamburgerMenu();
    void presentationChangesReportOnlyActualStateChanges();
    void storedValueChangesNormalizeAndReportOnlyActualChanges();
};

void TestApplicationMenuPresentationState::invalidStoredValuesNormalizeToHamburgerMenu()
{
    QCOMPARE(
        KiriView::ApplicationActions::ApplicationMenuPresentationState::presentationForStoredValue(
            -1),
        MenuPresentation::HamburgerMenu);
    QCOMPARE(
        KiriView::ApplicationActions::ApplicationMenuPresentationState::presentationForStoredValue(
            99),
        MenuPresentation::HamburgerMenu);
    QCOMPARE(
        KiriView::ApplicationActions::ApplicationMenuPresentationState::storedValueForPresentation(
            static_cast<MenuPresentation>(99)),
        static_cast<int>(MenuPresentation::HamburgerMenu));
}

void TestApplicationMenuPresentationState::presentationChangesReportOnlyActualStateChanges()
{
    KiriView::ApplicationActions::ApplicationMenuPresentationState state;

    QCOMPARE(state.presentation(), MenuPresentation::HamburgerMenu);
    QVERIFY(!state.setPresentation(MenuPresentation::HamburgerMenu));
    QVERIFY(state.setPresentation(MenuPresentation::MenuBar));
    QCOMPARE(state.presentation(), MenuPresentation::MenuBar);
    QCOMPARE(state.storedValue(), static_cast<int>(MenuPresentation::MenuBar));
    QVERIFY(!state.setPresentation(MenuPresentation::MenuBar));
}

void TestApplicationMenuPresentationState::storedValueChangesNormalizeAndReportOnlyActualChanges()
{
    KiriView::ApplicationActions::ApplicationMenuPresentationState state(MenuPresentation::MenuBar);

    QVERIFY(!state.setStoredValue(static_cast<int>(MenuPresentation::MenuBar)));
    QVERIFY(state.setStoredValue(99));
    QCOMPARE(state.presentation(), MenuPresentation::HamburgerMenu);
    QCOMPARE(state.storedValue(), static_cast<int>(MenuPresentation::HamburgerMenu));
    QVERIFY(!state.setStoredValue(-1));
}

QTEST_GUILESS_MAIN(TestApplicationMenuPresentationState)

#include "test_applicationmenupresentationstate.moc"
