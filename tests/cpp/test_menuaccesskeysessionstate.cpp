// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/menuaccesskeysessionstate.h"

#include <QObject>
#include <QTest>

class TestMenuAccessKeySessionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void beginSessionEventActivatesAccessKeyVisuals();
    void activeAltReleaseEventClearsAndConsumes();
    void inactiveAltReleaseEventOnlyClearsVisuals();
    void unavailableMenuEventClearsVisualsAndResetsSession();
    void clearSessionEventClearsVisualsWithoutConsuming();
    void clearSessionEventIsIdempotent();
    void clearSessionEventResetsPlainMnemonicSessionSnapshot();
    void altKeyRouteActivatesVisualsConsumesAndStartsSession();
    void altMnemonicRouteActivatesVisualsAndTriggersFromKeyPress();
    void shortcutOverrideClaimsAltMnemonicWithoutTriggering();
    void shortcutOverrideIgnoresPlainMnemonicsAndOtherKeys();
    void plainMnemonicRouteUsesCurrentSessionState();
};

namespace {
void comparePlan(const kiriview::MenuAccessKeySessionPlan &plan,
    kiriview::MenuAccessKeyVisualEffect visualEffect, bool consumeEvent, bool triggerMnemonic,
    bool accessKeySessionActive)
{
    QVERIFY(plan.visualEffect == visualEffect);
    QCOMPARE(plan.consumeEvent, consumeEvent);
    QCOMPARE(plan.triggerMnemonic, triggerMnemonic);
    QCOMPARE(plan.accessKeySessionActive, accessKeySessionActive);
}
}

void TestMenuAccessKeySessionState::beginSessionEventActivatesAccessKeyVisuals()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin),
        kiriview::MenuAccessKeyVisualEffect::Activate, false, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::activeAltReleaseEventClearsAndConsumes()
{
    kiriview::MenuAccessKeySessionState state;
    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::ReleaseAltKey),
        kiriview::MenuAccessKeyVisualEffect::Clear, true, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::inactiveAltReleaseEventOnlyClearsVisuals()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::ReleaseAltKey),
        kiriview::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::unavailableMenuEventClearsVisualsAndResetsSession()
{
    kiriview::MenuAccessKeySessionState state;
    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::MenuUnavailable),
        kiriview::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventClearsVisualsWithoutConsuming()
{
    kiriview::MenuAccessKeySessionState state;
    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Clear),
        kiriview::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventIsIdempotent()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Clear),
        kiriview::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventResetsPlainMnemonicSessionSnapshot()
{
    kiriview::MenuAccessKeySessionState state;
    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin);
    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Clear);

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::Mnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::KeyPress),
        kiriview::MenuAccessKeyVisualEffect::None, false, true, false);
}

void TestMenuAccessKeySessionState::altKeyRouteActivatesVisualsConsumesAndStartsSession()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::AltKey,
                    kiriview::MenuAccessKeyRoutingPhase::KeyPress),
        kiriview::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::altMnemonicRouteActivatesVisualsAndTriggersFromKeyPress()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::AltMnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::KeyPress),
        kiriview::MenuAccessKeyVisualEffect::Activate, false, true, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideClaimsAltMnemonicWithoutTriggering()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::AltMnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::ShortcutOverride),
        kiriview::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideIgnoresPlainMnemonicsAndOtherKeys()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::Mnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::ShortcutOverride),
        kiriview::MenuAccessKeyVisualEffect::None, false, false, false);
    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::Other,
                    kiriview::MenuAccessKeyRoutingPhase::ShortcutOverride),
        kiriview::MenuAccessKeyVisualEffect::None, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::plainMnemonicRouteUsesCurrentSessionState()
{
    kiriview::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::Mnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::KeyPress),
        kiriview::MenuAccessKeyVisualEffect::None, false, true, false);

    state.handleSessionEvent(kiriview::MenuAccessKeySessionEvent::Begin);
    comparePlan(state.routeOpenMenuKey(kiriview::MenuAccessKeyInputKind::Mnemonic,
                    kiriview::MenuAccessKeyRoutingPhase::KeyPress),
        kiriview::MenuAccessKeyVisualEffect::None, false, true, true);
}

QTEST_GUILESS_MAIN(TestMenuAccessKeySessionState)

#include "test_menuaccesskeysessionstate.moc"
