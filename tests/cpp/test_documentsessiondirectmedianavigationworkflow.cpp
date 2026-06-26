// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationworkflow.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionDirectMediaNavigationWorkflow : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void failedRefreshClearsNavigationAndRequestsProgrammaticReveal();
    void refreshRequestsRevealWhenSelectionChanges();
    void successfulOpenOfNewTargetKeepsPendingRevealAndRoutes();
    void successfulOpenWithoutTargetClearsPendingReveal();
};

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl& url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::ActiveNavigationSnapshot knownActiveNavigation(int currentNumber, int count)
{
    return kiriview::ActiveNavigationSnapshot {
        true,
        true,
        true,
        currentNumber > 1,
        currentNumber < count,
        currentNumber == 1,
        currentNumber == count,
        currentNumber,
        count,
    };
}
}

void TestDocumentSessionDirectMediaNavigationWorkflow::
    failedRefreshClearsNavigationAndRequestsProgrammaticReveal()
{
    const kiriview::DocumentSessionDirectMediaNavigationRefreshApplication application
        = kiriview::documentSessionDirectMediaNavigationRefreshApplication(
            kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, knownActiveNavigation(2, 3),
            kiriview::DocumentSessionDirectMediaNavigationRefreshResult {
                {}, {}, false, QStringLiteral("listing failed") });

    QVERIFY(!application.known);
    QVERIFY(application.candidates.empty());
    QCOMPARE(application.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    QVERIFY(!application.schedulePredecode);
}

void TestDocumentSessionDirectMediaNavigationWorkflow::refreshRequestsRevealWhenSelectionChanges()
{
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(previousUrl),
        directMediaNavigationCandidate(currentUrl),
    };
    const kiriview::DirectMediaNavigationBoundaryState boundary {
        true,
        false,
        false,
        true,
        2,
        2,
    };

    const kiriview::DocumentSessionDirectMediaNavigationRefreshApplication application
        = kiriview::documentSessionDirectMediaNavigationRefreshApplication(
            kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, knownActiveNavigation(1, 2),
            kiriview::DocumentSessionDirectMediaNavigationRefreshResult {
                candidates, boundary, true, QString() });

    QVERIFY(application.known);
    QCOMPARE(application.boundaryState.currentNumber, 2);
    QCOMPARE(application.candidates.size(), std::size_t(2));
    QCOMPARE(application.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::UsePendingOrProgrammaticSync);
    QVERIFY(application.schedulePredecode);
}

void TestDocumentSessionDirectMediaNavigationWorkflow::
    successfulOpenOfNewTargetKeepsPendingRevealAndRoutes()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(currentUrl),
        directMediaNavigationCandidate(nextUrl),
    };

    const kiriview::DocumentSessionDirectMediaNavigationOpenApplication application
        = kiriview::documentSessionDirectMediaNavigationOpenApplication(currentUrl,
            kiriview::DocumentSessionDirectMediaNavigationOpenResult {
                candidates,
                kiriview::DirectMediaNavigationOpenPlan {
                    kiriview::DirectMediaNavigationBoundaryState {
                        false,
                        true,
                        true,
                        false,
                        1,
                        2,
                    },
                    nextUrl,
                },
                true,
                QString(),
            });

    QVERIFY(application.known);
    QCOMPARE(application.routeTargetUrl.value_or(QUrl()), nextUrl);
    QCOMPARE(application.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::
            UsePendingOrProgrammaticSyncAndKeepPending);
    QVERIFY(application.schedulePredecode);
}

void TestDocumentSessionDirectMediaNavigationWorkflow::
    successfulOpenWithoutTargetClearsPendingReveal()
{
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.png"));

    const kiriview::DocumentSessionDirectMediaNavigationOpenApplication application
        = kiriview::documentSessionDirectMediaNavigationOpenApplication(currentUrl,
            kiriview::DocumentSessionDirectMediaNavigationOpenResult {
                { directMediaNavigationCandidate(currentUrl) },
                kiriview::DirectMediaNavigationOpenPlan {
                    kiriview::DirectMediaNavigationBoundaryState {
                        false,
                        false,
                        true,
                        true,
                        1,
                        1,
                    },
                    std::nullopt,
                },
                true,
                QString(),
            });

    QVERIFY(application.known);
    QVERIFY(!application.routeTargetUrl.has_value());
    QCOMPARE(application.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::Clear);
    QVERIFY(application.schedulePredecode);
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationWorkflow)

#include "test_documentsessiondirectmedianavigationworkflow.moc"
