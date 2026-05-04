// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heifcontainer.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QtGlobal>
#include <initializer_list>
#include <string_view>

namespace {
QByteArray heifFtypBox(std::string_view majorBrand, std::initializer_list<std::string_view> brands)
{
    const quint32 boxSize = 16 + static_cast<quint32>(brands.size() * 4);
    QByteArray data;
    data.append(static_cast<char>((boxSize >> 24) & 0xff));
    data.append(static_cast<char>((boxSize >> 16) & 0xff));
    data.append(static_cast<char>((boxSize >> 8) & 0xff));
    data.append(static_cast<char>(boxSize & 0xff));
    data.append("ftyp", 4);
    data.append(majorBrand.data(), 4);
    data.append(4, '\0');
    for (std::string_view brand : brands) {
        data.append(brand.data(), 4);
    }
    return data;
}
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
    QVERIFY(KiriView::isLikelyHeifContainer(avifStill));
    QVERIFY(KiriView::isLikelyHeifStillImageContainer(avifStill));
    QVERIFY(!KiriView::isLikelyHeifSequenceContainer(avifStill));

    const QByteArray avifSequence = heifFtypBox("avis", {});
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
    QVERIFY(!KiriView::isLikelyHeifContainer(nonHeif));
    QVERIFY(!KiriView::isLikelyHeifStillImageContainer(nonHeif));
    QVERIFY(!KiriView::isLikelyHeifSequenceContainer(nonHeif));
}

QTEST_GUILESS_MAIN(TestHeifContainer)

#include "test_heifcontainer.moc"
