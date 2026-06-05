// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/rasterdisplaybucketpolicy.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <QtGlobal>

class TestRasterDisplayBucketPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exactCurrentImageNeedsNoRefinement();
    void firstDisplayIsSufficientAtCurrentZoom();
    void lowerDetailCurrentImageRequestsSafeRefinementBucket();
    void unsafeSharperDemandKeepsCurrentImageAsBoundedDetail();
    void initialDemandWithoutSafeBucketIsUnsupported();
    void svgBucketsUseCoarseScaleSequence();
    void demandKeysMatchOnlyWhenEveryStaleRejectionFieldMatches();
};

namespace {
KiriView::RasterDisplayBucketPolicyInput policyInput()
{
    return KiriView::RasterDisplayBucketPolicyInput {
        QSize(1600, 1200),
        QSize(),
        KiriView::DisplayImageQuality::FirstDisplay,
        QSizeF(800.0, 600.0),
        QRectF(0.0, 0.0, 800.0, 600.0),
        1.0,
        0,
        4096,
        512 * 1024 * 1024,
    };
}
}

void TestRasterDisplayBucketPolicy::exactCurrentImageNeedsNoRefinement()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = input.originalSize;
    input.currentQuality = KiriView::DisplayImageQuality::Exact;

    const KiriView::RasterDisplayBucketDecision decision
        = KiriView::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::Exact);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, input.originalSize);
    QVERIFY(decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::firstDisplayIsSufficientAtCurrentZoom()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = QSize(800, 600);
    input.currentQuality = KiriView::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(400.0, 300.0);
    input.devicePixelRatio = 2.0;

    const KiriView::RasterDisplayBucketDecision decision
        = KiriView::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::FirstDisplaySufficient);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(800, 600));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::lowerDetailCurrentImageRequestsSafeRefinementBucket()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = QSize(400, 300);
    input.currentQuality = KiriView::DisplayImageQuality::ThumbnailPreview;
    input.displaySize = QSizeF(1000.0, 750.0);

    const KiriView::RasterDisplayBucketDecision decision
        = KiriView::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(1000, 750));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::unsafeSharperDemandKeepsCurrentImageAsBoundedDetail()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize(1000, 1000);
    input.currentQuality = KiriView::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.maximumTextureSize = 4096;

    const KiriView::RasterDisplayBucketDecision decision
        = KiriView::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::BoundedDetail);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::BoundedDetail);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(1000, 1000));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::initialDemandWithoutSafeBucketIsUnsupported()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize();
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.maximumTextureSize = 4096;
    input.displayImageByteBudget = 3;

    const KiriView::RasterDisplayBucketDecision decision
        = KiriView::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::UnsupportedTooLarge);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::Unsupported);
    QVERIFY(decision.bucketKey.rasterSize.isEmpty());
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(!decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::svgBucketsUseCoarseScaleSequence()
{
    KiriView::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(80, 40);
    input.currentRasterSize = QSize(80, 40);
    input.currentQuality = KiriView::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(100.0, 50.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 100.0, 50.0);

    KiriView::RasterDisplayBucketDecision decision = KiriView::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(120, 60));
    QVERIFY(decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);

    input.currentRasterSize = QSize(120, 60);
    input.currentQuality = KiriView::DisplayImageQuality::Exact;
    input.displaySize = QSizeF(110.0, 55.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 110.0, 55.0);

    decision = KiriView::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(120, 60));
    QVERIFY(!decision.refinementRequired);

    input.displaySize = QSizeF(150.0, 75.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 150.0, 75.0);

    decision = KiriView::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, KiriView::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(180, 90));
}

void TestRasterDisplayBucketPolicy::demandKeysMatchOnlyWhenEveryStaleRejectionFieldMatches()
{
    const KiriView::RasterDisplayBucketKey bucketKey {
        QSize(1000, 750),
        false,
        4096,
        512 * 1024 * 1024,
    };
    const KiriView::RasterDisplayRefinementDemandKey key {
        QStringLiteral("source-a"),
        QSize(1600, 1200),
        KiriView::DisplayedPageRole::Primary,
        7,
        11,
        13,
        17,
        19,
        23,
        29,
        bucketKey,
    };

    KiriView::RasterDisplayRefinementDemandKey same = key;
    KiriView::RasterDisplayRefinementDemandKey staleIntrinsicSize = key;
    staleIntrinsicSize.originalSize = QSize(1200, 1600);
    KiriView::RasterDisplayRefinementDemandKey staleZoom = key;
    staleZoom.zoomGeneration = 12;
    KiriView::RasterDisplayRefinementDemandKey staleRenderRevision = key;
    staleRenderRevision.renderRevision = 30;
    KiriView::RasterDisplayRefinementDemandKey staleBucket = key;
    staleBucket.bucketKey.rasterSize = QSize(1200, 900);

    QVERIFY(key == same);
    QVERIFY(key != staleIntrinsicSize);
    QVERIFY(key != staleZoom);
    QVERIFY(key != staleRenderRevision);
    QVERIFY(key != staleBucket);
}

QTEST_GUILESS_MAIN(TestRasterDisplayBucketPolicy)

#include "test_rasterdisplaybucketpolicy.moc"
