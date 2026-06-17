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
    void pageSlotSourceVariantsSeparateProviderReadyAndDisplayError();
    void primaryProjectionCombinesSlotScopeAndGeometry();
    void hiddenSecondaryProjectionKeepsRoleAndStatusOnly();
};

namespace {
kiriview::ImageDisplaySourceSlot displaySourceSlot(
    const QString &id, const QSize &originalSize, const QSize &rasterSize)
{
    return kiriview::ImageDisplaySourceSlot {
        QUrl(QStringLiteral("image://kiriview-images/%1").arg(id)),
        7,
        QStringLiteral("source-%1").arg(id),
        originalSize,
        rasterSize,
        QSize(rasterSize.width(), 0),
        kiriview::DisplayImageQuality::FirstDisplay,
        kiriview::ImageDisplaySourceStatus::Ready,
        false,
        false,
        kiriview::ImageDisplaySourceRetentionStatus::None,
        false,
    };
}
}

void TestImageDisplaySourceProjection::pageSlotSourceVariantsSeparateProviderReadyAndDisplayError()
{
    const QSize imageSize(100, 50);
    const kiriview::ImageDisplaySourceSlot readySource
        = displaySourceSlot(QStringLiteral("ready"), imageSize, QSize(25, 13));
    kiriview::ImageDisplaySourceSlot errorSource;
    errorSource.originalSize = imageSize;
    errorSource.rasterSize = imageSize;
    errorSource.status = kiriview::ImageDisplaySourceStatus::Error;

    const kiriview::ImagePresentationPageSlotSource ready
        = kiriview::ImagePresentationPageSlotSource::providerReady(readySource);
    const kiriview::ImagePresentationPageSlotSource error
        = kiriview::ImagePresentationPageSlotSource::displayError(errorSource);

    QCOMPARE(ready.kind(), kiriview::ImagePresentationPageSlotSourceKind::ProviderReady);
    QVERIFY(ready.hasImage());
    QCOMPARE(ready.displaySource().providerUrl, readySource.providerUrl);
    QCOMPARE(ready.displaySource().status, kiriview::ImageDisplaySourceStatus::Ready);
    QCOMPARE(error.kind(), kiriview::ImagePresentationPageSlotSourceKind::DisplayError);
    QVERIFY(error.hasImage());
    QVERIFY(error.displaySource().providerUrl.isEmpty());
    QCOMPARE(error.displaySource().status, kiriview::ImageDisplaySourceStatus::Error);
}

void TestImageDisplaySourceProjection::primaryProjectionCombinesSlotScopeAndGeometry()
{
    const QUrl imageUrl = kiriview::TestSupport::localUrl(QStringLiteral("/images/page.png"));
    kiriview::ImagePresentationRuntime runtime;
    runtime.setViewportSize(QSizeF(200.0, 120.0));

    kiriview::ImagePresentationPageSlotSnapshot slot;
    slot.imageSize = QSize(100, 50);
    slot.hasImage = true;
    slot.displaySource
        = displaySourceSlot(QStringLiteral("primary"), slot.imageSize, QSize(25, 13));
    runtime.commitPrimaryPageSlot(slot, kiriview::ImagePresentationScopeKey::directImage(imageUrl));
    runtime.rotateClockwise();

    const kiriview::ImageDisplaySourceProjection projection
        = runtime.displaySourceProjection(kiriview::DisplayedPageRole::Primary);

    QVERIFY(projection.visible);
    QCOMPARE(projection.pageRole, kiriview::DisplayedPageRole::Primary);
    QCOMPARE(projection.providerUrl, QUrl(QStringLiteral("image://kiriview-images/primary")));
    QCOMPARE(projection.revision, quint64(7));
    QCOMPARE(projection.revisionToken, QStringLiteral("7"));
    QCOMPARE(projection.sourceIdentity, QStringLiteral("source-primary"));
    QCOMPARE(projection.selectedSourceScope.kind,
        kiriview::ImagePresentationScopeKey::Kind::DirectImage);
    QCOMPARE(projection.selectedSourceScope.url, imageUrl);
    QCOMPARE(projection.originalSize, QSize(100, 50));
    QCOMPARE(projection.rasterSize, QSize(25, 13));
    QCOMPARE(projection.sourceSizeHint, QSize(25, 0));
    QCOMPARE(projection.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Ready);
    QVERIFY(!projection.cacheEnabled);
    QVERIFY(!projection.loadAcknowledgmentRequired);
    QCOMPARE(projection.rotationDegrees, 90);
    QCOMPARE(projection.retentionStatus, kiriview::ImageDisplaySourceRetentionStatus::None);
    QVERIFY(!projection.retainWhileLoadingEligible);
    QVERIFY(!projection.displaySize.isEmpty());
    QVERIFY(!projection.visibleItemRect.isEmpty());
}

void TestImageDisplaySourceProjection::hiddenSecondaryProjectionKeepsRoleAndStatusOnly()
{
    kiriview::ImagePresentationRuntime runtime;

    kiriview::ImagePresentationPageSlotSnapshot secondary;
    secondary.imageSize = QSize(80, 40);
    secondary.hasImage = true;
    secondary.displaySource
        = displaySourceSlot(QStringLiteral("secondary"), secondary.imageSize, QSize(80, 40));
    runtime.commitSecondaryPageSlot(secondary);

    const kiriview::ImageDisplaySourceProjection projection
        = runtime.displaySourceProjection(kiriview::DisplayedPageRole::Secondary);

    QVERIFY(!projection.visible);
    QCOMPARE(projection.pageRole, kiriview::DisplayedPageRole::Secondary);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Missing);
    QVERIFY(projection.providerUrl.isEmpty());
    QCOMPARE(projection.revision, quint64(0));
}

QTEST_GUILESS_MAIN(TestImageDisplaySourceProjection)

#include "test_imagedisplaysourceprojection.moc"
