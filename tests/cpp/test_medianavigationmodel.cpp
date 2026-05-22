// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/medianavigationmodel.h"

#include <QObject>
#include <QTest>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::MediaNavigationCandidate candidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}
}

class TestMediaNavigationModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentUrlUsesDirectImageCandidateContextRule();
    void navigatesMixedMediaWithoutWrapping();
    void deletionFallbackUsesOriginalDirectMediaUrl();
    void sortingAndMatchingNormalizePathSegmentsAndPercentEncoding();
    void boundaryStateIsUnknownWhenCurrentMediaIsMissing();
};

void TestMediaNavigationModel::parentUrlUsesDirectImageCandidateContextRule()
{
    QCOMPARE(KiriView::mediaNavigationParentUrl(localUrl(QStringLiteral("/media/a/../b/clip.mp4"))),
        localUrl(QStringLiteral("/media/b/")));

    const QUrl archiveEntry(QStringLiteral("zip:///path/archive.zip!/chapter/clip.mp4"));
    QCOMPARE(KiriView::mediaNavigationParentUrl(archiveEntry),
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")));
}

void TestMediaNavigationModel::navigatesMixedMediaWithoutWrapping()
{
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortMediaNavigationCandidates(&candidates);

    QCOMPARE(KiriView::adjacentMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), KiriView::NavigationDirection::Previous)
                 .value(),
        localUrl(QStringLiteral("/media/01.jpg")));
    QCOMPARE(KiriView::adjacentMediaNavigationUrl(candidates,
                 localUrl(QStringLiteral("/media/02.mp4")), KiriView::NavigationDirection::Next)
                 .value(),
        localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(!KiriView::adjacentMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/01.jpg")), KiriView::NavigationDirection::Previous));
    QVERIFY(!KiriView::adjacentMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/03.png")), KiriView::NavigationDirection::Next));
    QVERIFY(!KiriView::adjacentMediaNavigationUrl(candidates,
        localUrl(QStringLiteral("/media/missing.png")), KiriView::NavigationDirection::Next));

    const KiriView::MediaNavigationBoundaryState firstBoundary
        = KiriView::mediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/01.jpg")));
    QVERIFY(firstBoundary.atKnownFirst);
    QVERIFY(!firstBoundary.canOpenPrevious);
    QVERIFY(firstBoundary.canOpenNext);
    QCOMPARE(firstBoundary.currentNumber, 1);
    QCOMPARE(firstBoundary.count, 3);

    const KiriView::MediaNavigationBoundaryState videoBoundary
        = KiriView::mediaNavigationBoundaryState(
            candidates, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(videoBoundary.currentNumber, 2);
    QCOMPARE(videoBoundary.count, 3);
}

void TestMediaNavigationModel::deletionFallbackUsesOriginalDirectMediaUrl()
{
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortMediaNavigationCandidates(&candidates);

    const QUrl deletedUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const KiriView::MediaDeletionFallbackPlan plan
        = KiriView::mediaDeletionFallbackPlan(candidates, deletedUrl);

    QCOMPARE(plan.targetUrl, deletedUrl);
    QCOMPARE(plan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestMediaNavigationModel::sortingAndMatchingNormalizePathSegmentsAndPercentEncoding()
{
    const QUrl encoded(QStringLiteral("zip:///path/archive.zip!/chapter/a%20clip.mp4"));
    const QUrl normalized(QStringLiteral("zip:///path/archive.zip!/chapter/extra/../a%20clip.mp4"));
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        KiriView::MediaNavigationCandidate { encoded, QStringLiteral("a clip.mp4") },
        KiriView::MediaNavigationCandidate { normalized, QStringLiteral("a clip.mp4") },
    };

    KiriView::sortMediaNavigationCandidates(&candidates);
    QCOMPARE(candidates.size(), std::size_t(1));
    QVERIFY(KiriView::mediaNavigationCandidateIndex(candidates, normalized).has_value());
}

void TestMediaNavigationModel::boundaryStateIsUnknownWhenCurrentMediaIsMissing()
{
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        candidate(localUrl(QStringLiteral("/media/01.jpg"))),
        candidate(localUrl(QStringLiteral("/media/02.mp4"))),
    };
    KiriView::sortMediaNavigationCandidates(&candidates);

    const KiriView::MediaNavigationBoundaryState boundary = KiriView::mediaNavigationBoundaryState(
        candidates, localUrl(QStringLiteral("/media/missing.png")));
    QVERIFY(!boundary.canOpenPrevious);
    QVERIFY(!boundary.canOpenNext);
    QVERIFY(!boundary.atKnownFirst);
    QVERIFY(!boundary.atKnownLast);
    QCOMPARE(boundary.currentNumber, 0);
    QCOMPARE(boundary.count, 0);
}

QTEST_GUILESS_MAIN(TestMediaNavigationModel)

#include "test_medianavigationmodel.moc"
