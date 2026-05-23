// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationmenupresentationstate.h"

#include <QObject>
#include <QTest>

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
        KiriViewApplication::HamburgerMenu);
    QCOMPARE(
        KiriView::ApplicationActions::ApplicationMenuPresentationState::presentationForStoredValue(
            99),
        KiriViewApplication::HamburgerMenu);
    QCOMPARE(
        KiriView::ApplicationActions::ApplicationMenuPresentationState::storedValueForPresentation(
            static_cast<KiriViewApplication::MenuPresentation>(99)),
        static_cast<int>(KiriViewApplication::HamburgerMenu));
}

void TestApplicationMenuPresentationState::presentationChangesReportOnlyActualStateChanges()
{
    KiriView::ApplicationActions::ApplicationMenuPresentationState state;

    QCOMPARE(state.presentation(), KiriViewApplication::HamburgerMenu);
    QVERIFY(!state.setPresentation(KiriViewApplication::HamburgerMenu));
    QVERIFY(state.setPresentation(KiriViewApplication::MenuBar));
    QCOMPARE(state.presentation(), KiriViewApplication::MenuBar);
    QCOMPARE(state.storedValue(), static_cast<int>(KiriViewApplication::MenuBar));
    QVERIFY(!state.setPresentation(KiriViewApplication::MenuBar));
}

void TestApplicationMenuPresentationState::storedValueChangesNormalizeAndReportOnlyActualChanges()
{
    KiriView::ApplicationActions::ApplicationMenuPresentationState state(
        KiriViewApplication::MenuBar);

    QVERIFY(!state.setStoredValue(static_cast<int>(KiriViewApplication::MenuBar)));
    QVERIFY(state.setStoredValue(99));
    QCOMPARE(state.presentation(), KiriViewApplication::HamburgerMenu);
    QCOMPARE(state.storedValue(), static_cast<int>(KiriViewApplication::HamburgerMenu));
    QVERIFY(!state.setStoredValue(-1));
}

QTEST_GUILESS_MAIN(TestApplicationMenuPresentationState)

#include "test_applicationmenupresentationstate.moc"
