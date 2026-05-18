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
    void resetClearsSessionOwnership();
};

namespace {
void compareTransition(const KiriView::MenuAccessKeySessionTransition &transition,
    KiriView::MenuAccessKeyVisualEffect visualEffect, bool consumeEvent)
{
    QVERIFY(transition.visualEffect == visualEffect);
    QCOMPARE(transition.consumeEvent, consumeEvent);
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

void TestMenuAccessKeySessionState::resetClearsSessionOwnership()
{
    KiriView::MenuAccessKeySessionState state;
    state.beginSession();

    state.reset();

    QVERIFY(!state.isActive());
}

QTEST_GUILESS_MAIN(TestMenuAccessKeySessionState)

#include "test_menuaccesskeysessionstate.moc"
