// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/directmedianavigationmodel.h"

#include <QObject>
#include <QTest>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate candidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
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
    QCOMPARE(kiriview::directMediaNavigationParentUrl(
                 localUrl(QStringLiteral("/media/a/../b/clip.mp4"))),
        localUrl(QStringLiteral("/media/b/")));

    const QUrl archiveEntry(QStringLiteral("zip:///path/archive.zip!/chapter/clip.mp4"));
    QCOMPARE(kiriview::directMediaNavigationParentUrl(archiveEntry),
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")));
}

void TestDirectMediaNavigationModel::navigatesMixedMediaWithoutWrapping()
{
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    kiriview::sortDirectMediaNavigationCandidates(&candidates);

    QCOMPARE(kiriview::adjacentDirectMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), kiriview::NavigationDirection::Previous)
                 .value(),
        localUrl(QStringLiteral("/media/01.jpg")));
    QCOMPARE(kiriview::adjacentDirectMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), kiriview::NavigationDirection::Next)
                 .value(),
        localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(!kiriview::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/01.jpg")), kiriview::NavigationDirection::Previous));
    QVERIFY(!kiriview::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/03.png")), kiriview::NavigationDirection::Next));
    QVERIFY(!kiriview::adjacentDirectMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/missing.png")), kiriview::NavigationDirection::Next));

    const kiriview::DirectMediaNavigationBoundaryState firstBoundary
        = kiriview::directMediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/01.jpg")));
    QVERIFY(firstBoundary.atKnownFirst);
    QVERIFY(!firstBoundary.canOpenPrevious);
    QVERIFY(firstBoundary.canOpenNext);
    QCOMPARE(firstBoundary.currentNumber, 1);
    QCOMPARE(firstBoundary.count, 3);

    const kiriview::DirectMediaNavigationBoundaryState videoBoundary
        = kiriview::directMediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(videoBoundary.currentNumber, 2);
    QCOMPARE(videoBoundary.count, 3);
}

void TestDirectMediaNavigationModel::openPlanUpdatesBoundaryStateAndSelectsTargets()
{
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    kiriview::sortDirectMediaNavigationCandidates(&candidates);

    const kiriview::DirectMediaNavigationOpenPlan nextPlan
        = kiriview::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            kiriview::nextDirectMediaNavigationOpenRequest());
    QCOMPARE(nextPlan.targetUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(nextPlan.boundaryState.currentNumber, 2);
    QCOMPARE(nextPlan.boundaryState.count, 3);
    QVERIFY(nextPlan.boundaryState.canOpenPrevious);
    QVERIFY(nextPlan.boundaryState.canOpenNext);

    const kiriview::DirectMediaNavigationOpenPlan previousPlan
        = kiriview::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            kiriview::previousDirectMediaNavigationOpenRequest());
    QCOMPARE(previousPlan.targetUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));

    const kiriview::DirectMediaNavigationOpenPlan lowNumberPlan
        = kiriview::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            kiriview::numberedDirectMediaNavigationOpenRequest(-4));
    QCOMPARE(lowNumberPlan.targetUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));

    const kiriview::DirectMediaNavigationOpenPlan highNumberPlan
        = kiriview::directMediaNavigationOpenPlan(candidates,
            localUrl(QStringLiteral("/media/02.mp4")),
            kiriview::numberedDirectMediaNavigationOpenRequest(9));
    QCOMPARE(highNumberPlan.targetUrl.value(), localUrl(QStringLiteral("/media/03.png")));

    const kiriview::DirectMediaNavigationOpenPlan emptyPlan
        = kiriview::directMediaNavigationOpenPlan({}, localUrl(QStringLiteral("/media/02.mp4")),
            kiriview::numberedDirectMediaNavigationOpenRequest(1));
    QVERIFY(!emptyPlan.targetUrl.has_value());
    QCOMPARE(emptyPlan.boundaryState.count, 0);
}

void TestDirectMediaNavigationModel::sortingAndMatchingNormalizePathSegmentsAndPercentEncoding()
{
    const QUrl encoded(QStringLiteral("zip:///path/archive.zip!/chapter/a%20clip.mp4"));
    const QUrl normalized(QStringLiteral("zip:///path/archive.zip!/chapter/extra/../a%20clip.mp4"));
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        kiriview::DirectMediaNavigationCandidate { encoded, QStringLiteral("a clip.mp4") },
        kiriview::DirectMediaNavigationCandidate { normalized, QStringLiteral("a clip.mp4") },
    };

    kiriview::sortDirectMediaNavigationCandidates(&candidates);
    QCOMPARE(candidates.size(), std::size_t(1));
    QVERIFY(kiriview::directMediaNavigationCandidateIndex(candidates, normalized).has_value());
}

void TestDirectMediaNavigationModel::boundaryStateIsUnknownWhenCurrentMediaIsMissing()
{
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
    };
    kiriview::sortDirectMediaNavigationCandidates(&candidates);

    const kiriview::DirectMediaNavigationBoundaryState boundary
        = kiriview::directMediaNavigationBoundaryState(
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
