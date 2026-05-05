// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifcontainer.h"

#include "image_test_support.h"

#include <QByteArray>
#include <QObject>
#include <QTest>

namespace {
using KiriView::TestSupport::heifFtypBox;
}

class TestHeifContainer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesStillAndSequenceBrands();
    void detectsMajorAndCompatibleBrands();
};

void TestHeifContainer::classifiesStillAndSequenceBrands()
{
    QCOMPARE(KiriView::heifBrandKind("avif"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("avci"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("mif1"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("heic"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("heif"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("hej2"), KiriView::HeifBrandKind::StillImage);
    QCOMPARE(KiriView::heifBrandKind("jpeg"), KiriView::HeifBrandKind::StillImage);

    QCOMPARE(KiriView::heifBrandKind("avis"), KiriView::HeifBrandKind::ImageSequence);
    QCOMPARE(KiriView::heifBrandKind("avcs"), KiriView::HeifBrandKind::ImageSequence);
    QCOMPARE(KiriView::heifBrandKind("hevc"), KiriView::HeifBrandKind::ImageSequence);
    QCOMPARE(KiriView::heifBrandKind("hevs"), KiriView::HeifBrandKind::ImageSequence);
    QCOMPARE(KiriView::heifBrandKind("msf1"), KiriView::HeifBrandKind::ImageSequence);

    QCOMPARE(KiriView::heifBrandKind("png "), KiriView::HeifBrandKind::Unknown);
}

void TestHeifContainer::detectsMajorAndCompatibleBrands()
{
    const QByteArray avifStill = heifFtypBox("avif", {});
    const KiriView::HeifContainerInfo avifStillInfo = KiriView::heifContainerInfo(avifStill);
    QVERIFY(avifStillInfo.isHeif());
    QVERIFY(avifStillInfo.stillImage);
    QVERIFY(!avifStillInfo.imageSequence);
    QVERIFY(KiriView::isLikelyHeifContainer(avifStill));
    QVERIFY(KiriView::isLikelyHeifStillImageContainer(avifStill));
    QVERIFY(!KiriView::isLikelyHeifSequenceContainer(avifStill));

    const QByteArray avifSequence = heifFtypBox("avis", {});
    const KiriView::HeifContainerInfo avifSequenceInfo = KiriView::heifContainerInfo(avifSequence);
    QVERIFY(avifSequenceInfo.isHeif());
    QVERIFY(!avifSequenceInfo.stillImage);
    QVERIFY(avifSequenceInfo.imageSequence);
    QVERIFY(KiriView::isLikelyHeifContainer(avifSequence));
    QVERIFY(!KiriView::isLikelyHeifStillImageContainer(avifSequence));
    QVERIFY(KiriView::isLikelyHeifSequenceContainer(avifSequence));

    const QByteArray compatibleStill = heifFtypBox("zzzz", { "mif1" });
    QVERIFY(KiriView::isLikelyHeifContainer(compatibleStill));
    QVERIFY(KiriView::isLikelyHeifStillImageContainer(compatibleStill));
    QVERIFY(!KiriView::isLikelyHeifSequenceContainer(compatibleStill));

    const QByteArray compatibleSequence = heifFtypBox("zzzz", { "avcs" });
    QVERIFY(KiriView::isLikelyHeifContainer(compatibleSequence));
    QVERIFY(!KiriView::isLikelyHeifStillImageContainer(compatibleSequence));
    QVERIFY(KiriView::isLikelyHeifSequenceContainer(compatibleSequence));

    const QByteArray nonHeif = heifFtypBox("png ", {});
    const KiriView::HeifContainerInfo nonHeifInfo = KiriView::heifContainerInfo(nonHeif);
    QVERIFY(!nonHeifInfo.isHeif());
    QVERIFY(!nonHeifInfo.stillImage);
    QVERIFY(!nonHeifInfo.imageSequence);
    QVERIFY(!KiriView::isLikelyHeifContainer(nonHeif));
    QVERIFY(!KiriView::isLikelyHeifStillImageContainer(nonHeif));
    QVERIFY(!KiriView::isLikelyHeifSequenceContainer(nonHeif));
}

QTEST_GUILESS_MAIN(TestHeifContainer)

#include "test_heifcontainer.moc"
