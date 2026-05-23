// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediacandidateloadstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionMediaCandidateLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onlyCurrentLoadCanFinish();
    void cursorScopeMustMatchCurrentLoad();
    void cancelRejectsPendingLoad();
};

void TestDocumentSessionMediaCandidateLoadState::onlyCurrentLoadCanFinish()
{
    KiriView::DocumentSessionMediaCandidateLoadState state;

    const KiriView::DocumentSessionMediaCandidateLoad stale
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")),
            QUrl::fromLocalFile(QStringLiteral("/media/")), 1);
    const KiriView::DocumentSessionMediaCandidateLoad current
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/02.mp4")),
            QUrl::fromLocalFile(QStringLiteral("/media/")), 2);

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(!state.accepts(stale));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.accepts(current));
    QVERIFY(state.finish(current));
    QVERIFY(!state.accepts(current));
}

void TestDocumentSessionMediaCandidateLoadState::cursorScopeMustMatchCurrentLoad()
{
    KiriView::DocumentSessionMediaCandidateLoadState state;

    const KiriView::DocumentSessionMediaCandidateLoad first
        = state.start(QUrl::fromLocalFile(QStringLiteral("/first/01.mp4")),
            QUrl::fromLocalFile(QStringLiteral("/first/")), 1);
    const KiriView::DocumentSessionMediaCandidateLoad second
        = state.start(QUrl::fromLocalFile(QStringLiteral("/second/01.mp4")),
            QUrl::fromLocalFile(QStringLiteral("/second/")), 2);

    KiriView::DocumentSessionMediaCandidateLoad wrongScope = second;
    wrongScope.parentUrl = first.parentUrl;
    KiriView::DocumentSessionMediaCandidateLoad wrongGeneration = second;
    wrongGeneration.cursorGeneration = first.cursorGeneration;

    QVERIFY(!state.accepts(first));
    QVERIFY(!state.finish(first));
    QVERIFY(!state.accepts(wrongScope));
    QVERIFY(!state.finish(wrongScope));
    QVERIFY(!state.accepts(wrongGeneration));
    QVERIFY(!state.finish(wrongGeneration));
    QVERIFY(state.accepts(second));
    QVERIFY(state.finish(second));
}

void TestDocumentSessionMediaCandidateLoadState::cancelRejectsPendingLoad()
{
    KiriView::DocumentSessionMediaCandidateLoadState state;
    const KiriView::DocumentSessionMediaCandidateLoad load
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")),
            QUrl::fromLocalFile(QStringLiteral("/media/")), 1);

    QVERIFY(state.accepts(load));
    state.cancel();
    QVERIFY(!state.accepts(load));
    QVERIFY(!state.finish(load));
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaCandidateLoadState)

#include "test_documentsessionmediacandidateloadstate.moc"
