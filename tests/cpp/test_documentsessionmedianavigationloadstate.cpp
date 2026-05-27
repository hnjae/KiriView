// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmedianavigationloadstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionMediaNavigationLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onlyCurrentLoadCanFinish();
    void cursorScopeMustMatchCurrentLoad();
    void cancelRejectsPendingLoad();
};

namespace {
KiriView::DirectMediaScope mediaScope(
    const QString &currentPath, const QString &parentPath, quint64 generation)
{
    return KiriView::DirectMediaScope {
        QUrl::fromLocalFile(currentPath),
        QUrl::fromLocalFile(parentPath),
        generation,
    };
}
}

void TestDocumentSessionMediaNavigationLoadState::onlyCurrentLoadCanFinish()
{
    KiriView::DocumentSessionMediaNavigationLoadState state;

    const KiriView::DocumentSessionMediaNavigationLoad stale
        = state.start(mediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));
    const KiriView::DocumentSessionMediaNavigationLoad current
        = state.start(mediaScope(QStringLiteral("/media/02.mp4"), QStringLiteral("/media/"), 2));

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(!state.accepts(stale));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.accepts(current));
    QVERIFY(state.finish(current));
    QVERIFY(!state.accepts(current));
}

void TestDocumentSessionMediaNavigationLoadState::cursorScopeMustMatchCurrentLoad()
{
    KiriView::DocumentSessionMediaNavigationLoadState state;

    const KiriView::DocumentSessionMediaNavigationLoad first
        = state.start(mediaScope(QStringLiteral("/first/01.mp4"), QStringLiteral("/first/"), 1));
    const KiriView::DocumentSessionMediaNavigationLoad second
        = state.start(mediaScope(QStringLiteral("/second/01.mp4"), QStringLiteral("/second/"), 2));

    KiriView::DocumentSessionMediaNavigationLoad wrongScope = second;
    wrongScope.scope.parentUrl = first.scope.parentUrl;
    KiriView::DocumentSessionMediaNavigationLoad wrongCurrent = second;
    wrongCurrent.scope.currentUrl = first.scope.currentUrl;
    KiriView::DocumentSessionMediaNavigationLoad wrongGeneration = second;
    wrongGeneration.scope.generation = first.scope.generation;

    QVERIFY(!state.accepts(first));
    QVERIFY(!state.finish(first));
    QVERIFY(!state.accepts(wrongScope));
    QVERIFY(!state.finish(wrongScope));
    QVERIFY(!state.accepts(wrongCurrent));
    QVERIFY(!state.finish(wrongCurrent));
    QVERIFY(!state.accepts(wrongGeneration));
    QVERIFY(!state.finish(wrongGeneration));
    QVERIFY(state.accepts(second));
    QVERIFY(state.finish(second));
}

void TestDocumentSessionMediaNavigationLoadState::cancelRejectsPendingLoad()
{
    KiriView::DocumentSessionMediaNavigationLoadState state;
    const KiriView::DocumentSessionMediaNavigationLoad load
        = state.start(mediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));

    QVERIFY(state.accepts(load));
    state.cancel();
    QVERIFY(!state.accepts(load));
    QVERIFY(!state.finish(load));
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaNavigationLoadState)

#include "test_documentsessionmedianavigationloadstate.moc"
