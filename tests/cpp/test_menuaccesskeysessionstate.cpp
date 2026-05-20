// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeysessionstate.h"

#include <QObject>
#include <QTest>

class TestMenuAccessKeySessionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void beginSessionActivatesAccessKeyVisuals();
    void activeAltReleaseClearsVisualsConsumesEventAndResetsSession();
    void inactiveAltReleaseOnlyClearsVisuals();
    void unavailableMenuClearsVisualsWithoutResettingActiveSession();
    void altKeyRouteActivatesVisualsConsumesAndStartsSession();
    void altMnemonicRouteActivatesVisualsAndTriggersFromKeyPress();
    void shortcutOverrideClaimsAltMnemonicWithoutTriggering();
    void shortcutOverrideIgnoresPlainMnemonicsAndOtherKeys();
    void plainMnemonicRouteUsesCurrentSessionState();
    void resetClearsSessionOwnership();
};

namespace {
void compareTransition(const KiriView::MenuAccessKeySessionTransition &transition,
    KiriView::MenuAccessKeyVisualEffect visualEffect, bool consumeEvent)
{
    QVERIFY(transition.visualEffect == visualEffect);
    QCOMPARE(transition.consumeEvent, consumeEvent);
}

void compareRoutePlan(const KiriView::MenuAccessKeyRoutePlan &plan,
    KiriView::MenuAccessKeyVisualEffect visualEffect, bool consumeEvent, bool triggerMnemonic,
    bool accessKeySessionActive)
{
    QVERIFY(plan.visualEffect == visualEffect);
    QCOMPARE(plan.consumeEvent, consumeEvent);
    QCOMPARE(plan.triggerMnemonic, triggerMnemonic);
    QCOMPARE(plan.accessKeySessionActive, accessKeySessionActive);
}
}

void TestMenuAccessKeySessionState::beginSessionActivatesAccessKeyVisuals()
{
    KiriView::MenuAccessKeySessionState state;

    compareTransition(state.beginSession(), KiriView::MenuAccessKeyVisualEffect::Activate, false);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::activeAltReleaseClearsVisualsConsumesEventAndResetsSession()
{
    KiriView::MenuAccessKeySessionState state;
    state.beginSession();

    compareTransition(state.releaseAltKey(), KiriView::MenuAccessKeyVisualEffect::Clear, true);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::inactiveAltReleaseOnlyClearsVisuals()
{
    KiriView::MenuAccessKeySessionState state;

    compareTransition(state.releaseAltKey(), KiriView::MenuAccessKeyVisualEffect::Clear, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::unavailableMenuClearsVisualsWithoutResettingActiveSession()
{
    KiriView::MenuAccessKeySessionState state;
    state.beginSession();

    compareTransition(state.menuUnavailable(), KiriView::MenuAccessKeyVisualEffect::Clear, false);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::altKeyRouteActivatesVisualsConsumesAndStartsSession()
{
    KiriView::MenuAccessKeySessionState state;

    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltKey,
                         KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::altMnemonicRouteActivatesVisualsAndTriggersFromKeyPress()
{
    KiriView::MenuAccessKeySessionState state;

    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltMnemonic,
                         KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::Activate, false, true, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideClaimsAltMnemonicWithoutTriggering()
{
    KiriView::MenuAccessKeySessionState state;

    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::AltMnemonic,
                         KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::Activate, true, false, true);
    QVERIFY(state.isActive());
}

void TestMenuAccessKeySessionState::shortcutOverrideIgnoresPlainMnemonicsAndOtherKeys()
{
    KiriView::MenuAccessKeySessionState state;

    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                         KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::None, false, false, false);
    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Other,
                         KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride),
        KiriView::MenuAccessKeyVisualEffect::None, false, false, false);
    QVERIFY(!state.isActive());
}

void TestMenuAccessKeySessionState::plainMnemonicRouteUsesCurrentSessionState()
{
    KiriView::MenuAccessKeySessionState state;

    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                         KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::None, false, true, false);

    state.beginSession();
    compareRoutePlan(state.routeOpenMenuKey(KiriView::MenuAccessKeyInputKind::Mnemonic,
                         KiriView::MenuAccessKeyRoutingPhase::KeyPress),
        KiriView::MenuAccessKeyVisualEffect::None, false, true, true);
}

void TestMenuAccessKeySessionState::resetClearsSessionOwnership()
{
    KiriView::MenuAccessKeySessionState state;
    state.beginSession();

    state.reset();

    QVERIFY(!state.isActive());
}

QTEST_GUILESS_MAIN(TestMenuAccessKeySessionState)

#include "test_menuaccesskeysessionstate.moc"
