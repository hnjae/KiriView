// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationloadstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionDirectMediaNavigationLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void onlyCurrentLoadCanFinish();
    void cursorScopeMustMatchCurrentLoad();
    void cancelRejectsPendingLoad();
};

namespace {
KiriView::DirectMediaScope directMediaScope(
    const QString &currentPath, const QString &parentPath, quint64 generation)
{
    return KiriView::DirectMediaScope {
        QUrl::fromLocalFile(currentPath),
        QUrl::fromLocalFile(parentPath),
        generation,
    };
}
}

void TestDocumentSessionDirectMediaNavigationLoadState::onlyCurrentLoadCanFinish()
{
    KiriView::DocumentSessionDirectMediaNavigationLoadState state;

    const KiriView::DocumentSessionDirectMediaNavigationLoad stale = state.start(
        directMediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));
    const KiriView::DocumentSessionDirectMediaNavigationLoad current = state.start(
        directMediaScope(QStringLiteral("/media/02.mp4"), QStringLiteral("/media/"), 2));

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(!state.accepts(stale));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.accepts(current));
    QVERIFY(state.finish(current));
    QVERIFY(!state.accepts(current));
}

void TestDocumentSessionDirectMediaNavigationLoadState::cursorScopeMustMatchCurrentLoad()
{
    KiriView::DocumentSessionDirectMediaNavigationLoadState state;

    const KiriView::DocumentSessionDirectMediaNavigationLoad first = state.start(
        directMediaScope(QStringLiteral("/first/01.mp4"), QStringLiteral("/first/"), 1));
    const KiriView::DocumentSessionDirectMediaNavigationLoad second = state.start(
        directMediaScope(QStringLiteral("/second/01.mp4"), QStringLiteral("/second/"), 2));

    KiriView::DocumentSessionDirectMediaNavigationLoad wrongScope = second;
    wrongScope.scope.parentUrl = first.scope.parentUrl;
    KiriView::DocumentSessionDirectMediaNavigationLoad wrongCurrent = second;
    wrongCurrent.scope.currentUrl = first.scope.currentUrl;
    KiriView::DocumentSessionDirectMediaNavigationLoad wrongGeneration = second;
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

void TestDocumentSessionDirectMediaNavigationLoadState::cancelRejectsPendingLoad()
{
    KiriView::DocumentSessionDirectMediaNavigationLoadState state;
    const KiriView::DocumentSessionDirectMediaNavigationLoad load = state.start(
        directMediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));

    QVERIFY(state.accepts(load));
    state.cancel();
    QVERIFY(!state.accepts(load));
    QVERIFY(!state.finish(load));
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationLoadState)

#include "test_documentsessiondirectmedianavigationloadstate.moc"
