// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationthumbnaildemand.h"

#include <QObject>
#include <QTest>
#include <QUrl>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestActiveNavigationThumbnailDemand : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void bucketPolicyMapsPhysicalEdge();
    void trackerCoalescesByBucketPrioritySourceAndGeneration();
};

void TestActiveNavigationThumbnailDemand::bucketPolicyMapsPhysicalEdge()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;

    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(-1), Bucket::None);
    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(0), Bucket::None);
    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(1), Bucket::Normal);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(127), Bucket::Normal);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(128), Bucket::Normal);
    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(129), Bucket::Large);
    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(255), Bucket::Large);
    QCOMPARE(KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(256), Bucket::Large);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(257), Bucket::XLarge);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(511), Bucket::XLarge);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(512), Bucket::XLarge);
    QCOMPARE(
        KiriView::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(513), Bucket::XXLarge);
}

void TestActiveNavigationThumbnailDemand::trackerCoalescesByBucketPrioritySourceAndGeneration()
{
    using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
    using Priority = KiriView::ActiveNavigationThumbnailDemandPriority;

    KiriView::ActiveNavigationThumbnailDemandTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    KiriView::ActiveNavigationThumbnailDemand demand {
        1,
        firstUrl,
        Bucket::Normal,
        Priority::Visible,
        7,
    };

    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));

    demand.bucket = Bucket::Large;
    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));

    demand.priority = Priority::Nearby;
    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));

    demand.navigationGeneration = 8;
    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));

    demand.url = secondUrl;
    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));

    demand.number = 2;
    QVERIFY(tracker.record(demand));
    QVERIFY(!tracker.record(demand));
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailDemand)

#include "test_activenavigationthumbnaildemand.moc"
