// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/directmedianavigationmodel.h"

#include <QObject>
#include <QTest>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::DirectMediaNavigationCandidate candidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}
}

class TestDirectMediaNavigationModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentUrlUsesDirectImageDocumentPageCandidateContextRule();
    void navigatesMixedMediaWithoutWrapping();
    void openPlanUpdatesBoundaryStateAndSelectsTargets();
    void sortingAndMatchingNormalizePathSegmentsAndPercentEncoding();
    void boundaryStateIsUnknownWhenCurrentMediaIsMissing();
};

void TestDirectMediaNavigationModel::parentUrlUsesDirectImageDocumentPageCandidateContextRule()
{
    QCOMPARE(KiriView::directMediaNavigationParentUrl(
                 localUrl(QStringLiteral("/media/a/../b/clip.mp4"))),
        localUrl(QStringLiteral("/media/b/")));

    const QUrl archiveEntry(QStringLiteral("zip:///path/archive.zip!/chapter/clip.mp4"));
    QCOMPARE(KiriView::directMediaNavigationParentUrl(archiveEntry),
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")));
}

void TestDirectMediaNavigationModel::navigatesMixedMediaWithoutWrapping()
{
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortDirectMediaNavigationCandidates(&candidates);

    QCOMPARE(KiriView::adjacentDirectMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), KiriView::NavigationDirection::Previous)
                 .value(),
        localUrl(QStringLiteral("/media/01.jpg")));
    QCOMPARE(KiriView::adjacentDirectMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), KiriView::NavigationDirection::Next)
                 .value(),
        localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(!KiriView::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/01.jpg")), KiriView::NavigationDirection::Previous));
    QVERIFY(!KiriView::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/03.png")), KiriView::NavigationDirection::Next));
    QVERIFY(!KiriView::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/missing.png")), KiriView::NavigationDirection::Next));

    const KiriView::DirectMediaNavigationBoundaryState firstBoundary
        = KiriView::directMediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/01.jpg")));
    QVERIFY(firstBoundary.atKnownFirst);
    QVERIFY(!firstBoundary.canOpenPrevious);
    QVERIFY(firstBoundary.canOpenNext);
    QCOMPARE(firstBoundary.currentNumber, 1);
    QCOMPARE(firstBoundary.count, 3);

    const KiriView::DirectMediaNavigationBoundaryState videoBoundary
        = KiriView::directMediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(videoBoundary.currentNumber, 2);
    QCOMPARE(videoBoundary.count, 3);
}

void TestDirectMediaNavigationModel::openPlanUpdatesBoundaryStateAndSelectsTargets()
{
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortDirectMediaNavigationCandidates(&candidates);

    const KiriView::DirectMediaNavigationOpenPlan nextPlan
        = KiriView::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            KiriView::nextDirectMediaNavigationOpenRequest());
    QCOMPARE(nextPlan.targetUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(nextPlan.boundaryState.currentNumber, 2);
    QCOMPARE(nextPlan.boundaryState.count, 3);
    QVERIFY(nextPlan.boundaryState.canOpenPrevious);
    QVERIFY(nextPlan.boundaryState.canOpenNext);

    const KiriView::DirectMediaNavigationOpenPlan previousPlan
        = KiriView::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            KiriView::previousDirectMediaNavigationOpenRequest());
    QCOMPARE(previousPlan.targetUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));

    const KiriView::DirectMediaNavigationOpenPlan lowNumberPlan
        = KiriView::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            KiriView::numberedDirectMediaNavigationOpenRequest(-4));
    QCOMPARE(lowNumberPlan.targetUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));

    const KiriView::DirectMediaNavigationOpenPlan highNumberPlan
        = KiriView::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            KiriView::numberedDirectMediaNavigationOpenRequest(9));
    QCOMPARE(highNumberPlan.targetUrl.value(), localUrl(QStringLiteral("/media/03.png")));

    const KiriView::DirectMediaNavigationOpenPlan emptyPlan
        = KiriView::directMediaNavigationOpenPlan({}, localUrl(QStringLiteral("/media/02.mp4")),
            KiriView::numberedDirectMediaNavigationOpenRequest(1));
    QVERIFY(!emptyPlan.targetUrl.has_value());
    QCOMPARE(emptyPlan.boundaryState.count, 0);
}

void TestDirectMediaNavigationModel::sortingAndMatchingNormalizePathSegmentsAndPercentEncoding()
{
    const QUrl encoded(QStringLiteral("zip:///path/archive.zip!/chapter/a%20clip.mp4"));
    const QUrl normalized(QStringLiteral("zip:///path/archive.zip!/chapter/extra/../a%20clip.mp4"));
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        KiriView::DirectMediaNavigationCandidate { encoded, QStringLiteral("a clip.mp4") },
        KiriView::DirectMediaNavigationCandidate { normalized, QStringLiteral("a clip.mp4") },
    };

    KiriView::sortDirectMediaNavigationCandidates(&candidates);
    QCOMPARE(candidates.size(), std::size_t(1));
    QVERIFY(KiriView::directMediaNavigationCandidateIndex(candidates, normalized).has_value());
}

void TestDirectMediaNavigationModel::boundaryStateIsUnknownWhenCurrentMediaIsMissing()
{
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
    };
    KiriView::sortDirectMediaNavigationCandidates(&candidates);

    const KiriView::DirectMediaNavigationBoundaryState boundary
        = KiriView::directMediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/missing.png")));
    QVERIFY(!boundary.canOpenPrevious);
    QVERIFY(!boundary.canOpenNext);
    QVERIFY(!boundary.atKnownFirst);
    QVERIFY(!boundary.atKnownLast);
    QCOMPARE(boundary.currentNumber, 0);
    QCOMPARE(boundary.count, 0);
}

QTEST_GUILESS_MAIN(TestDirectMediaNavigationModel)

#include "test_directmedianavigationmodel.moc"
