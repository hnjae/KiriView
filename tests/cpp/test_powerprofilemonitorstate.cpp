// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/powerprofilemonitorstate.h"

#include <QObject>
#include <QTest>

class TestPowerProfileMonitorState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void powerSaverValueEventOwnsCanonicalStateChanges();
    void invalidationEventRequestsRefreshWithoutChangingState();
    void ignoredEventLeavesStateUnchanged();
};

void TestPowerProfileMonitorState::powerSaverValueEventOwnsCanonicalStateChanges()
{
    KiriView::PowerProfileMonitorState state;
    QVERIFY(!state.powerSaverEnabled());

    KiriView::PowerProfileMonitorPlan plan
        = state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());

    plan = state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());

    plan = state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverValue(false));
    QVERIFY(plan.powerSaverChanged);
    QVERIFY(!state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::invalidationEventRequestsRefreshWithoutChangingState()
{
    KiriView::PowerProfileMonitorState state;
    state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverValue(true));

    const KiriView::PowerProfileMonitorPlan plan
        = state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverInvalidated());

    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::ignoredEventLeavesStateUnchanged()
{
    KiriView::PowerProfileMonitorState state;
    state.applyEvent(KiriView::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(state.powerSaverEnabled());

    const KiriView::PowerProfileMonitorPlan plan
        = state.applyEvent(KiriView::PowerProfileMonitorEvent::ignore());

    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());
}

QTEST_GUILESS_MAIN(TestPowerProfileMonitorState)

#include "test_powerprofilemonitorstate.moc"
