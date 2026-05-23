// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imageinputclassificationconversion.h"

#include <QObject>
#include <QTest>

class TestImageInputClassificationConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageInputClassificationProjectsBridgeEnums();
};

void TestImageInputClassificationConversion::imageInputClassificationProjectsBridgeEnums()
{
    const KiriView::ImageInputClassification svg
        = KiriView::imageInputClassificationFromBridge(KiriView::RustImageInputClassification {
            KiriView::RustImageInputKind::Svg,
            KiriView::RustQtRasterFormat::None,
            KiriView::RustImageDecodeDataSource::Original,
        });
    QVERIFY(svg.kind == KiriView::ImageInputKind::Svg);
    QVERIFY(svg.qtFormat == KiriView::QtRasterFormat::None);
    QVERIFY(svg.dataSource == KiriView::ImageDecodeDataSource::Original);

    const KiriView::ImageInputClassification heif
        = KiriView::imageInputClassificationFromBridge(KiriView::RustImageInputClassification {
            KiriView::RustImageInputKind::HeifFamily,
            KiriView::RustQtRasterFormat::None,
            KiriView::RustImageDecodeDataSource::AvifCompatible,
        });
    QVERIFY(heif.kind == KiriView::ImageInputKind::HeifFamily);
    QVERIFY(heif.qtFormat == KiriView::QtRasterFormat::None);
    QVERIFY(heif.dataSource == KiriView::ImageDecodeDataSource::AvifCompatible);

    const KiriView::ImageInputClassification jxl
        = KiriView::imageInputClassificationFromBridge(KiriView::RustImageInputClassification {
            KiriView::RustImageInputKind::QtRaster,
            KiriView::RustQtRasterFormat::Jxl,
            KiriView::RustImageDecodeDataSource::Original,
        });
    QVERIFY(jxl.kind == KiriView::ImageInputKind::QtRaster);
    QVERIFY(jxl.qtFormat == KiriView::QtRasterFormat::Jxl);
    QVERIFY(jxl.dataSource == KiriView::ImageDecodeDataSource::Original);
}

QTEST_GUILESS_MAIN(TestImageInputClassificationConversion)

#include "test_imageinputclassificationconversion.moc"
