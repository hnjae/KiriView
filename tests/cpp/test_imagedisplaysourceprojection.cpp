// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagedisplaysourceprojection.h"
#include "presentation/imagepresentationruntime.h"

#include "image_test_support.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>

class TestImageDisplaySourceProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void primaryProjectionCombinesSlotScopeAndGeometry();
    void hiddenSecondaryProjectionKeepsRoleAndStatusOnly();
};

namespace {
KiriView::ImageDisplaySourceSlot displaySourceSlot(
    const QString &id, const QSize &originalSize, const QSize &rasterSize)
{
    return KiriView::ImageDisplaySourceSlot {
        QUrl(QStringLiteral("image://kiriview-images/%1").arg(id)),
        7,
        QStringLiteral("source-%1").arg(id),
        originalSize,
        rasterSize,
        QSize(rasterSize.width(), 0),
        KiriView::DisplayImageQuality::FirstDisplay,
        KiriView::ImageDisplaySourceStatus::Ready,
        false,
        false,
        KiriView::ImageDisplaySourceRetentionStatus::None,
        false,
    };
}
}

void TestImageDisplaySourceProjection::primaryProjectionCombinesSlotScopeAndGeometry()
{
    const QUrl imageUrl = KiriView::TestSupport::localUrl(QStringLiteral("/images/page.png"));
    KiriView::ImagePresentationRuntime runtime;
    runtime.setViewportSize(QSizeF(200.0, 120.0));

    KiriView::ImagePresentationPageSlotSnapshot slot;
    slot.imageSize = QSize(100, 50);
    slot.hasImage = true;
    slot.displaySource
        = displaySourceSlot(QStringLiteral("primary"), slot.imageSize, QSize(25, 13));
    runtime.commitPrimaryPageSlot(slot, KiriView::ImagePresentationScopeKey::directImage(imageUrl));
    runtime.rotateClockwise();

    const KiriView::ImageDisplaySourceProjection projection
        = runtime.displaySourceProjection(KiriView::DisplayedPageRole::Primary);

    QVERIFY(projection.visible);
    QCOMPARE(projection.pageRole, KiriView::DisplayedPageRole::Primary);
    QCOMPARE(projection.providerUrl, QUrl(QStringLiteral("image://kiriview-images/primary")));
    QCOMPARE(projection.revision, quint64(7));
    QCOMPARE(projection.revisionToken, QStringLiteral("7"));
    QCOMPARE(projection.sourceIdentity, QStringLiteral("source-primary"));
    QCOMPARE(projection.selectedSourceScope.kind,
        KiriView::ImagePresentationScopeKey::Kind::DirectImage);
    QCOMPARE(projection.selectedSourceScope.url, imageUrl);
    QCOMPARE(projection.originalSize, QSize(100, 50));
    QCOMPARE(projection.rasterSize, QSize(25, 13));
    QCOMPARE(projection.sourceSizeHint, QSize(25, 0));
    QCOMPARE(projection.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(projection.status, KiriView::ImageDisplaySourceStatus::Ready);
    QVERIFY(!projection.cacheEnabled);
    QVERIFY(!projection.loadAcknowledgmentRequired);
    QCOMPARE(projection.rotationDegrees, 90);
    QCOMPARE(projection.retentionStatus, KiriView::ImageDisplaySourceRetentionStatus::None);
    QVERIFY(!projection.retainWhileLoadingEligible);
    QVERIFY(!projection.displaySize.isEmpty());
    QCOMPARE(projection.visibleItemRect, QRectF());
}

void TestImageDisplaySourceProjection::hiddenSecondaryProjectionKeepsRoleAndStatusOnly()
{
    KiriView::ImagePresentationRuntime runtime;

    KiriView::ImagePresentationPageSlotSnapshot secondary;
    secondary.imageSize = QSize(80, 40);
    secondary.hasImage = true;
    secondary.displaySource
        = displaySourceSlot(QStringLiteral("secondary"), secondary.imageSize, QSize(80, 40));
    runtime.commitSecondaryPageSlot(secondary);

    const KiriView::ImageDisplaySourceProjection projection
        = runtime.displaySourceProjection(KiriView::DisplayedPageRole::Secondary);

    QVERIFY(!projection.visible);
    QCOMPARE(projection.pageRole, KiriView::DisplayedPageRole::Secondary);
    QCOMPARE(projection.status, KiriView::ImageDisplaySourceStatus::Missing);
    QVERIFY(projection.providerUrl.isEmpty());
    QCOMPARE(projection.revision, quint64(0));
}

QTEST_GUILESS_MAIN(TestImageDisplaySourceProjection)

#include "test_imagedisplaysourceprojection.moc"
