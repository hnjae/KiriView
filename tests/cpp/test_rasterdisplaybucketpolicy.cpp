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
    void svgBucketsApplyRotationBeforeChoosingScale();
    void svgUnsafeSharperDemandKeepsCurrentImageAsBoundedDetail();
    void svgInitialDemandWithoutSafeBucketIsUnsupported();
    void demandKeysMatchOnlyWhenEveryStaleRejectionFieldMatches();
};

namespace {
kiriview::RasterDisplayBucketPolicyInput policyInput()
{
    return kiriview::RasterDisplayBucketPolicyInput {
        QSize(1600, 1200),
        QSize(),
        kiriview::DisplayImageQuality::FirstDisplay,
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
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = input.originalSize;
    input.currentQuality = kiriview::DisplayImageQuality::Exact;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::Exact);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, input.originalSize);
    QVERIFY(decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::firstDisplayIsSufficientAtCurrentZoom()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = QSize(800, 600);
    input.currentQuality = kiriview::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(400.0, 300.0);
    input.devicePixelRatio = 2.0;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::FirstDisplaySufficient);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(800, 600));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::lowerDetailCurrentImageRequestsSafeRefinementBucket()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.currentRasterSize = QSize(400, 300);
    input.currentQuality = kiriview::DisplayImageQuality::ThumbnailPreview;
    input.displaySize = QSizeF(1000.0, 750.0);

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(1000, 750));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::unsafeSharperDemandKeepsCurrentImageAsBoundedDetail()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize(1000, 1000);
    input.currentQuality = kiriview::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.maximumTextureSize = 4096;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::BoundedDetail);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::BoundedDetail);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(1000, 1000));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::initialDemandWithoutSafeBucketIsUnsupported()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize();
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.maximumTextureSize = 4096;
    input.displayImageByteBudget = 3;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::rasterDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::UnsupportedTooLarge);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Unsupported);
    QVERIFY(decision.bucketKey.rasterSize.isEmpty());
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(!decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::svgBucketsUseCoarseScaleSequence()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(80, 40);
    input.currentRasterSize = QSize(80, 40);
    input.currentQuality = kiriview::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(100.0, 50.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 100.0, 50.0);

    kiriview::RasterDisplayBucketDecision decision = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(120, 60));
    QVERIFY(decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);

    input.currentRasterSize = QSize(120, 60);
    input.currentQuality = kiriview::DisplayImageQuality::Exact;
    input.displaySize = QSizeF(110.0, 55.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 110.0, 55.0);

    decision = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(120, 60));
    QVERIFY(!decision.refinementRequired);

    input.displaySize = QSizeF(150.0, 75.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 150.0, 75.0);

    decision = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(180, 90));
}

void TestRasterDisplayBucketPolicy::svgBucketsApplyRotationBeforeChoosingScale()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(80, 40);
    input.currentRasterSize = QSize(80, 40);
    input.currentQuality = kiriview::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(50.0, 100.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 50.0, 100.0);
    input.rotationDegrees = 90;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::RefinementNeeded);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(120, 60));
    QVERIFY(decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::svgUnsafeSharperDemandKeepsCurrentImageAsBoundedDetail()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize(1000, 1000);
    input.currentQuality = kiriview::DisplayImageQuality::FirstDisplay;
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 10000.0, 10000.0);
    input.maximumTextureSize = 4096;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::BoundedDetail);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::BoundedDetail);
    QCOMPARE(decision.bucketKey.rasterSize, QSize(1000, 1000));
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::svgInitialDemandWithoutSafeBucketIsUnsupported()
{
    kiriview::RasterDisplayBucketPolicyInput input = policyInput();
    input.originalSize = QSize(10000, 10000);
    input.currentRasterSize = QSize();
    input.displaySize = QSizeF(10000.0, 10000.0);
    input.visibleItemRect = QRectF(0.0, 0.0, 10000.0, 10000.0);
    input.maximumTextureSize = 4096;
    input.displayImageByteBudget = 3;

    const kiriview::RasterDisplayBucketDecision decision
        = kiriview::svgDisplayBucketDecision(input);

    QCOMPARE(decision.status, kiriview::RasterDisplayBucketStatus::UnsupportedTooLarge);
    QCOMPARE(decision.quality, kiriview::DisplayImageQuality::Unsupported);
    QVERIFY(decision.bucketKey.rasterSize.isEmpty());
    QVERIFY(!decision.bucketKey.exact);
    QVERIFY(!decision.refinementRequired);
    QVERIFY(!decision.currentImageRetained);
}

void TestRasterDisplayBucketPolicy::demandKeysMatchOnlyWhenEveryStaleRejectionFieldMatches()
{
    const kiriview::RasterDisplayBucketKey bucketKey {
        QSize(1000, 750),
        false,
        4096,
        512 * 1024 * 1024,
    };
    const kiriview::RasterDisplayRefinementDemandKey key {
        QStringLiteral("source-a"),
        QSize(1600, 1200),
        kiriview::DisplayedPageRole::Primary,
        7,
        11,
        13,
        17,
        19,
        23,
        29,
        bucketKey,
    };

    kiriview::RasterDisplayRefinementDemandKey same = key;
    kiriview::RasterDisplayRefinementDemandKey staleIntrinsicSize = key;
    staleIntrinsicSize.originalSize = QSize(1200, 1600);
    kiriview::RasterDisplayRefinementDemandKey staleZoom = key;
    staleZoom.zoomGeneration = 12;
    kiriview::RasterDisplayRefinementDemandKey staleRenderRevision = key;
    staleRenderRevision.renderRevision = 30;
    kiriview::RasterDisplayRefinementDemandKey staleBucket = key;
    staleBucket.bucketKey.rasterSize = QSize(1200, 900);

    QVERIFY(key == same);
    QVERIFY(key != staleIntrinsicSize);
    QVERIFY(key != staleZoom);
    QVERIFY(key != staleRenderRevision);
    QVERIFY(key != staleBucket);
}

QTEST_GUILESS_MAIN(TestRasterDisplayBucketPolicy)

#include "test_rasterdisplaybucketpolicy.moc"
