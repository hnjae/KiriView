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
void comparePlan(const KiriView::MenuAccessKeySessionPlan &plan,
    KiriView::MenuAccessKeyVisualEffect visualEffect, bool consumeEvent, bool triggerMnemonic,
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
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin),
        KiriView::MenuAccessKeyVisualEffect::Activate, false, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::activeAltReleaseEventClearsAndConsumes()
{
    KiriView::MenuAccessKeySessionState state;
    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::ReleaseAltKey),
        KiriView::MenuAccessKeyVisualEffect::Clear, true, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::inactiveAltReleaseEventOnlyClearsVisuals()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::ReleaseAltKey),
        KiriView::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::unavailableMenuEventClearsVisualsAndResetsSession()
{
    KiriView::MenuAccessKeySessionState state;
    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::MenuUnavailable),
        KiriView::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventClearsVisualsWithoutConsuming()
{
    KiriView::MenuAccessKeySessionState state;
    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin);

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Clear),
        KiriView::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventIsIdempotent()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Clear),
        KiriView::MenuAccessKeyVisualEffect::Clear, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::clearSessionEventResetsPlainMnemonicSessionSnapshot()
{
    KiriView::MenuAccessKeySessionState state;
    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin);
    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Clear);

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::None, false, true, false);
}

void TestMenuAccessKeySessionState::altKeyRouteActivatesVisualsConsumesAndStartsSession()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltKey,
                    KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::altMnemonicRouteActivatesVisualsAndTriggersFromKeyPress()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltMnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::Activate, false, true, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideClaimsAltMnemonicWithoutTriggering()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltMnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideIgnoresPlainMnemonicsAndOtherKeys()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::None, false, false, false);
    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Other,
                    KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::None, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::plainMnemonicRouteUsesCurrentSessionState()
{
    KiriView::MenuAccessKeySessionState state;

    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::None, false, true, false);

    state.handleSessionEvent(KiriView::MenuAccessKeySessionEvent::Begin);
    comparePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                    KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::None, false, true, true);
}

QTEST_GUILESS_MAIN(TestMenuAccessKeySessionState)

#include "test_menuaccesskeysessionstate.moc"
