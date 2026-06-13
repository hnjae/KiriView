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
    kiriview::PowerProfileMonitorState state;
    QVERIFY(!state.powerSaverEnabled());

    kiriview::PowerProfileMonitorPlan plan
        = state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());

    plan = state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());

    plan = state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverValue(false));
    QVERIFY(plan.powerSaverChanged);
    QVERIFY(!state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::invalidationEventRequestsRefreshWithoutChangingState()
{
    kiriview::PowerProfileMonitorState state;
    state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverValue(true));

    const kiriview::PowerProfileMonitorPlan plan
        = state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverInvalidated());

    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());
}

void TestPowerProfileMonitorState::ignoredEventLeavesStateUnchanged()
{
    kiriview::PowerProfileMonitorState state;
    state.applyEvent(kiriview::PowerProfileMonitorEvent::powerSaverValue(true));
    QVERIFY(state.powerSaverEnabled());

    const kiriview::PowerProfileMonitorPlan plan
        = state.applyEvent(kiriview::PowerProfileMonitorEvent::ignore());

    QVERIFY(!plan.powerSaverChanged);
    QVERIFY(!plan.refreshPowerSaverEnabled);
    QVERIFY(state.powerSaverEnabled());
}

QTEST_GUILESS_MAIN(TestPowerProfileMonitorState)

#include "test_powerprofilemonitorstate.moc"
