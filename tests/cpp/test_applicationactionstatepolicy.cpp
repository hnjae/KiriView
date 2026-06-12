// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationactionstatepolicy.h"

#include <QObject>
#include <QTest>

namespace {
using ActionId = KiriView::ApplicationActions::ActionId;

KiriView::ApplicationActions::ApplicationActionStateInput readyImageInput()
{
    KiriView::ApplicationActions::ApplicationActionStateInput input;
    input.helpActionsEnabled = true;
    input.readyActionsEnabled = true;
    input.rotateActionsEnabled = true;
    input.twoPageModeActionsEnabled = true;
    input.rightToLeftReadingActionsEnabled = true;
    input.containerNavigationActionsEnabled = true;
    input.displayedMediaOpenWithAvailable = true;
    input.displayedFileDeletionAvailable = true;
    input.activeNavigationAvailable = true;
    input.activeNavigationKnown = true;
    input.activeNavigationHasTargets = true;
    input.canOpenPreviousActiveNavigation = true;
    input.canOpenNextActiveNavigation = true;
    return input;
}

KiriView::ApplicationActions::ApplicationActionState stateFor(
    ActionId actionId, const KiriView::ApplicationActions::ApplicationActionStateInput &input)
{
    return KiriView::ApplicationActions::applicationActionState(actionId, input);
}
}

class TestApplicationActionStatePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visiblePreviousNextPlacementsDisableAtBoundaries();
    void sharedActionEnabledStateUsesRuntimeGates();
    void disabledStableActionsKeepPlacement();
    void checkedStateComesFromRuntimeInputs();
};

void TestApplicationActionStatePolicy::visiblePreviousNextPlacementsDisableAtBoundaries()
{
    auto input = readyImageInput();
    input.canOpenPreviousActiveNavigation = false;
    input.canOpenNextActiveNavigation = true;

    const KiriView::ApplicationActions::ApplicationActionState previousState
        = stateFor(ActionId::GoPreviousImageAction, input);
    const KiriView::ApplicationActions::ApplicationActionState nextState
        = stateFor(ActionId::GoNextImageAction, input);

    QVERIFY(previousState.actionEnabled);
    QVERIFY(!previousState.placementEnabled);
    QVERIFY(nextState.actionEnabled);
    QVERIFY(nextState.placementEnabled);

    input.activeNavigationKnown = false;
    QCOMPARE(stateFor(ActionId::GoPreviousImageAction, input).actionEnabled, false);
    QCOMPARE(stateFor(ActionId::GoPreviousImageAction, input).placementEnabled, false);
}

void TestApplicationActionStatePolicy::sharedActionEnabledStateUsesRuntimeGates()
{
    auto input = readyImageInput();

    QVERIFY(stateFor(ActionId::FileOpenAction, input).actionEnabled);
    QVERIFY(stateFor(ActionId::FileOpenWithAction, input).actionEnabled);
    QVERIFY(stateFor(ActionId::FileMoveToTrashAction, input).actionEnabled);
    QVERIFY(stateFor(ActionId::ViewZoomInAction, input).actionEnabled);
    QVERIFY(stateFor(ActionId::ViewRotateClockwiseAction, input).actionEnabled);
    QVERIFY(stateFor(ActionId::GoPreviousArchiveAction, input).actionEnabled);

    input.helpActionsEnabled = false;
    QVERIFY(!stateFor(ActionId::FileOpenAction, input).actionEnabled);
    QVERIFY(!stateFor(ActionId::FileOpenWithAction, input).actionEnabled);
    QVERIFY(!stateFor(ActionId::GoPreviousImageAction, input).actionEnabled);
}

void TestApplicationActionStatePolicy::disabledStableActionsKeepPlacement()
{
    auto input = readyImageInput();
    input.helpActionsEnabled = false;
    input.readyActionsEnabled = false;
    input.twoPageModeActionsEnabled = false;
    input.rightToLeftReadingActionsEnabled = false;

    const KiriView::ApplicationActions::ApplicationActionState rightToLeftState
        = stateFor(ActionId::ViewToggleRightToLeftReadingAction, input);
    const KiriView::ApplicationActions::ApplicationActionState twoPageState
        = stateFor(ActionId::ViewToggleTwoPageModeAction, input);
    const KiriView::ApplicationActions::ApplicationActionState openState
        = stateFor(ActionId::FileOpenAction, input);

    QVERIFY(!rightToLeftState.actionEnabled);
    QVERIFY(rightToLeftState.placementEnabled);
    QVERIFY(!twoPageState.actionEnabled);
    QVERIFY(twoPageState.placementEnabled);
    QVERIFY(!openState.actionEnabled);
    QVERIFY(openState.placementEnabled);
}

void TestApplicationActionStatePolicy::checkedStateComesFromRuntimeInputs()
{
    auto input = readyImageInput();
    input.fitModeSelected = true;
    input.twoPageModeActive = true;
    input.rightToLeftReadingActive = true;
    input.infoPanelVisible = true;
    input.fullscreen = true;

    QCOMPARE(stateFor(ActionId::ViewFitAction, input).checked, true);
    QCOMPARE(stateFor(ActionId::ViewFitHeightAction, input).checked, false);
    QCOMPARE(stateFor(ActionId::ViewToggleTwoPageModeAction, input).checked, true);
    QCOMPARE(stateFor(ActionId::ViewToggleRightToLeftReadingAction, input).checked, true);
    QCOMPARE(stateFor(ActionId::ViewToggleInfoPanelAction, input).checked, true);
    QCOMPARE(stateFor(ActionId::WindowFullscreenAction, input).checked, true);
}

QTEST_GUILESS_MAIN(TestApplicationActionStatePolicy)

#include "test_applicationactionstatepolicy.moc"
