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
    void sourceKeyEquivalentScopeCanFinish();
    void cancelRejectsPendingLoad();
};

namespace {
kiriview::DirectMediaScope directMediaScope(
    const QString& currentPath, const QString& parentPath, quint64 generation)
{
    return kiriview::DirectMediaScope {
        QUrl::fromLocalFile(currentPath),
        QUrl::fromLocalFile(parentPath),
        generation,
    };
}
}

void TestDocumentSessionDirectMediaNavigationLoadState::onlyCurrentLoadCanFinish()
{
    kiriview::DocumentSessionDirectMediaNavigationLoadState state;

    const kiriview::DocumentSessionDirectMediaNavigationLoad stale = state.start(
        directMediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));
    const kiriview::DocumentSessionDirectMediaNavigationLoad current = state.start(
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
    kiriview::DocumentSessionDirectMediaNavigationLoadState state;

    const kiriview::DocumentSessionDirectMediaNavigationLoad first = state.start(
        directMediaScope(QStringLiteral("/first/01.mp4"), QStringLiteral("/first/"), 1));
    const kiriview::DocumentSessionDirectMediaNavigationLoad second = state.start(
        directMediaScope(QStringLiteral("/second/01.mp4"), QStringLiteral("/second/"), 2));

    kiriview::DocumentSessionDirectMediaNavigationLoad wrongScope = second;
    wrongScope.scope.parentUrl = first.scope.parentUrl;
    kiriview::DocumentSessionDirectMediaNavigationLoad wrongCurrent = second;
    wrongCurrent.scope.currentUrl = first.scope.currentUrl;
    kiriview::DocumentSessionDirectMediaNavigationLoad wrongGeneration = second;
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

void TestDocumentSessionDirectMediaNavigationLoadState::sourceKeyEquivalentScopeCanFinish()
{
    kiriview::DocumentSessionDirectMediaNavigationLoadState state;

    kiriview::DocumentSessionDirectMediaNavigationLoad load
        = state.start(kiriview::DirectMediaScope {
            QUrl(QStringLiteral("file:///media/chapter/../01.mp4")),
            QUrl(QStringLiteral("file:///media/chapter/..")),
            3,
        });

    load.scope.currentUrl = QUrl(QStringLiteral("file:///media/01.mp4"));
    load.scope.parentUrl = QUrl(QStringLiteral("file:///media/"));
    QVERIFY(state.accepts(load));

    kiriview::DocumentSessionDirectMediaNavigationLoad wrongGeneration = load;
    wrongGeneration.scope.generation = 4;
    QVERIFY(!state.accepts(wrongGeneration));
    QVERIFY(state.finish(load));
}

void TestDocumentSessionDirectMediaNavigationLoadState::cancelRejectsPendingLoad()
{
    kiriview::DocumentSessionDirectMediaNavigationLoadState state;
    const kiriview::DocumentSessionDirectMediaNavigationLoad load = state.start(
        directMediaScope(QStringLiteral("/media/01.mp4"), QStringLiteral("/media/"), 1));

    QVERIFY(state.accepts(load));
    state.cancel();
    QVERIFY(!state.accepts(load));
    QVERIFY(!state.finish(load));
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationLoadState)

#include "test_documentsessiondirectmedianavigationloadstate.moc"
