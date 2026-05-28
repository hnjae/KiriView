// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/mediapredecodeeligibility.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}
}

class TestMediaPredecodeEligibility : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void snapshotKeepsMixedMediaIndicesForStillImages();
    void targetIndicesReturnOnlyEligibleStillImages();
};

void TestMediaPredecodeEligibility::snapshotKeepsMixedMediaIndicesForStillImages()
{
    const QUrl previousImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl currentVideo = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    const QUrl laterVideo = localUrl(QStringLiteral("/media/03.mov"));
    const QUrl laterImage = localUrl(QStringLiteral("/media/04.jpg"));

    const KiriView::MediaPredecodeEligibilitySnapshot snapshot
        = KiriView::mediaPredecodeEligibilitySnapshot(
            {
                directMediaNavigationCandidate(previousImage),
                directMediaNavigationCandidate(currentVideo),
                directMediaNavigationCandidate(nextImage),
                directMediaNavigationCandidate(laterVideo),
                directMediaNavigationCandidate(laterImage),
            },
            currentVideo);

    QCOMPARE(snapshot.directMediaNavigationCandidateCount, std::size_t(5));
    QVERIFY(snapshot.currentMediaIndex.has_value());
    QCOMPARE(*snapshot.currentMediaIndex, std::size_t(1));
    QCOMPARE(snapshot.images.size(), std::size_t(3));
    QCOMPARE(snapshot.images.at(0).url, previousImage);
    QCOMPARE(snapshot.images.at(0).mediaIndex, std::size_t(0));
    QCOMPARE(snapshot.images.at(1).url, nextImage);
    QCOMPARE(snapshot.images.at(1).mediaIndex, std::size_t(2));
    QCOMPARE(snapshot.images.at(2).url, laterImage);
    QCOMPARE(snapshot.images.at(2).mediaIndex, std::size_t(4));
}

void TestMediaPredecodeEligibility::targetIndicesReturnOnlyEligibleStillImages()
{
    const QUrl previousImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl currentVideo = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    const QUrl laterImage = localUrl(QStringLiteral("/media/04.jpg"));
    const KiriView::MediaPredecodeEligibilitySnapshot snapshot
        = KiriView::mediaPredecodeEligibilitySnapshot(
            {
                directMediaNavigationCandidate(previousImage),
                directMediaNavigationCandidate(currentVideo),
                directMediaNavigationCandidate(nextImage),
                directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.mov"))),
                directMediaNavigationCandidate(laterImage),
            },
            currentVideo);

    const std::vector<QUrl> urls
        = KiriView::mediaPredecodeEligibleUrlsForTargetIndices(snapshot, { 2, 1, 4, 99, 0 });

    QCOMPARE(urls.size(), std::size_t(3));
    QCOMPARE(urls.at(0), nextImage);
    QCOMPARE(urls.at(1), laterImage);
    QCOMPARE(urls.at(2), previousImage);
}

QTEST_GUILESS_MAIN(TestMediaPredecodeEligibility)

#include "test_mediapredecodeeligibility.moc"
