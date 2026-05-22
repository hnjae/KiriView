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
    void cancelRejectsPendingLoad();
};

void TestDocumentSessionMediaCandidateLoadState::onlyCurrentLoadCanFinish()
{
    KiriView::DocumentSessionMediaCandidateLoadState state;

    const KiriView::DocumentSessionMediaCandidateLoad stale
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")));
    const KiriView::DocumentSessionMediaCandidateLoad current
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/02.mp4")));

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(!state.accepts(stale));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.accepts(current));
    QVERIFY(state.finish(current));
    QVERIFY(!state.accepts(current));
}

void TestDocumentSessionMediaCandidateLoadState::cancelRejectsPendingLoad()
{
    KiriView::DocumentSessionMediaCandidateLoadState state;
    const KiriView::DocumentSessionMediaCandidateLoad load
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")));

    QVERIFY(state.accepts(load));
    state.cancel();
    QVERIFY(!state.accepts(load));
    QVERIFY(!state.finish(load));
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaCandidateLoadState)

#include "test_documentsessionmediacandidateloadstate.moc"
