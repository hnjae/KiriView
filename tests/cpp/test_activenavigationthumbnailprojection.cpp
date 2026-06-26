// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationthumbnailprojection.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <vector>

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(
    const QUrl& url, const QString& name = {})
{
    return kiriview::DirectMediaNavigationCandidate { url, name };
}

kiriview::ImageDocumentPageTarget imageTarget(const QUrl& url,
    kiriview::ImageDocumentPageKind kind = kiriview::ImageDocumentPageKind::Image,
    const QString& name = {})
{
    return kiriview::ImageDocumentPageTarget(url, kind, name);
}

kiriview::ActiveNavigationSnapshot knownNavigation(int currentNumber, int count)
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

class TestActiveNavigationThumbnailProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directMediaRowsUseConfirmedCandidates();
    void imageDocumentRowsUsePageSnapshot();
    void unavailableUnknownAndMismatchedNavigationProjectNoRows();
};

void TestActiveNavigationThumbnailProjection::directMediaRowsUseConfirmedCandidates()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));

    const std::vector<kiriview::ActiveNavigationThumbnailRow> rows
        = kiriview::projectActiveNavigationThumbnailRows(
            kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, knownNavigation(2, 2),
            {
                directMediaNavigationCandidate(imageUrl),
                directMediaNavigationCandidate(videoUrl, QStringLiteral("Clip")),
            },
            {});

    QCOMPARE(rows.size(), std::size_t(2));
    QCOMPARE(rows.at(0).number, 1);
    QCOMPARE(rows.at(0).url, imageUrl);
    QCOMPARE(rows.at(0).label, QStringLiteral("01.png"));
    QVERIFY(rows.at(0).kind == kiriview::ActiveNavigationThumbnailKind::Image);
    QVERIFY(!rows.at(0).current);

    QCOMPARE(rows.at(1).number, 2);
    QCOMPARE(rows.at(1).url, videoUrl);
    QCOMPARE(rows.at(1).label, QStringLiteral("Clip"));
    QVERIFY(rows.at(1).kind == kiriview::ActiveNavigationThumbnailKind::Video);
    QVERIFY(rows.at(1).current);
}

void TestActiveNavigationThumbnailProjection::imageDocumentRowsUsePageSnapshot()
{
    const QUrl firstPage = localUrl(QStringLiteral("/archive/01.png"));
    const QUrl secondPage = localUrl(QStringLiteral("/archive/clip.mp4"));
    kiriview::ImageDocumentPageNavigationSnapshot pageSnapshot;
    pageSnapshot.state = kiriview::PageNavigationState(
        {
            imageTarget(firstPage, kiriview::ImageDocumentPageKind::Image,
                QStringLiteral("chapter/01.png")),
            imageTarget(secondPage, kiriview::ImageDocumentPageKind::Video),
        },
        0);

    const std::vector<kiriview::ActiveNavigationThumbnailRow> rows
        = kiriview::projectActiveNavigationThumbnailRows(
            kiriview::ActiveNavigationSourceKind::ImageDocumentPages, knownNavigation(1, 2), {},
            pageSnapshot);

    QCOMPARE(rows.size(), std::size_t(2));
    QCOMPARE(rows.at(0).number, 1);
    QCOMPARE(rows.at(0).url, firstPage);
    QCOMPARE(rows.at(0).label, QStringLiteral("chapter/01.png"));
    QVERIFY(rows.at(0).kind == kiriview::ActiveNavigationThumbnailKind::Image);
    QVERIFY(rows.at(0).current);

    QCOMPARE(rows.at(1).number, 2);
    QCOMPARE(rows.at(1).url, secondPage);
    QCOMPARE(rows.at(1).label, QStringLiteral("clip.mp4"));
    QVERIFY(rows.at(1).kind == kiriview::ActiveNavigationThumbnailKind::Video);
    QVERIFY(!rows.at(1).current);
}

void TestActiveNavigationThumbnailProjection::
    unavailableUnknownAndMismatchedNavigationProjectNoRows()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(imageUrl)
    };

    QVERIFY(kiriview::projectActiveNavigationThumbnailRows(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, candidates, {})
            .empty());

    kiriview::ActiveNavigationSnapshot unknown;
    unknown.available = true;
    QVERIFY(kiriview::projectActiveNavigationThumbnailRows(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, unknown, candidates, {})
            .empty());

    QVERIFY(kiriview::projectActiveNavigationThumbnailRows(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, knownNavigation(1, 2),
        candidates, {})
            .empty());
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailProjection)

#include "test_activenavigationthumbnailprojection.moc"
