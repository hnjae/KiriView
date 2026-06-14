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
    void trackerCoalescesRepeatedDemandPerRowAfterOtherRows();
};

void TestActiveNavigationThumbnailDemand::bucketPolicyMapsPhysicalEdge()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;

    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(-1), Bucket::None);
    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(0), Bucket::None);
    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(1), Bucket::Normal);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(127), Bucket::Normal);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(128), Bucket::Normal);
    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(129), Bucket::Large);
    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(255), Bucket::Large);
    QCOMPARE(kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(256), Bucket::Large);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(257), Bucket::XLarge);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(511), Bucket::XLarge);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(512), Bucket::XLarge);
    QCOMPARE(
        kiriview::activeNavigationThumbnailDemandBucketForPhysicalMaxEdge(513), Bucket::XXLarge);
}

void TestActiveNavigationThumbnailDemand::trackerCoalescesByBucketPrioritySourceAndGeneration()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    kiriview::ActiveNavigationThumbnailDemandTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    kiriview::ActiveNavigationThumbnailDemand demand {
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

void TestActiveNavigationThumbnailDemand::trackerCoalescesRepeatedDemandPerRowAfterOtherRows()
{
    using Bucket = kiriview::ActiveNavigationThumbnailDemandBucket;
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;

    kiriview::ActiveNavigationThumbnailDemandTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/02.png"));
    kiriview::ActiveNavigationThumbnailDemand firstDemand {
        1,
        firstUrl,
        Bucket::Normal,
        Priority::Visible,
        7,
    };
    kiriview::ActiveNavigationThumbnailDemand secondDemand {
        2,
        secondUrl,
        Bucket::Normal,
        Priority::Visible,
        7,
    };

    QVERIFY(tracker.record(firstDemand));
    QVERIFY(tracker.record(secondDemand));
    QVERIFY(!tracker.record(firstDemand));

    firstDemand.bucket = Bucket::Large;
    QVERIFY(tracker.record(firstDemand));
    QVERIFY(!tracker.record(firstDemand));

    firstDemand.bucket = Bucket::Normal;
    QVERIFY(tracker.record(firstDemand));
    QVERIFY(!tracker.record(firstDemand));
}

QTEST_GUILESS_MAIN(TestActiveNavigationThumbnailDemand)

#include "test_activenavigationthumbnaildemand.moc"
