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
    const kiriview::ImageInputClassification svg
        = kiriview::imageInputClassificationFromBridge(kiriview::RustImageInputClassification {
            kiriview::RustImageInputKind::Svg,
            kiriview::RustQtRasterFormat::None,
            kiriview::RustImageDecodeDataSource::Original,
        });
    QVERIFY(svg.kind == kiriview::ImageInputKind::Svg);
    QVERIFY(svg.qtFormat == kiriview::QtRasterFormat::None);
    QVERIFY(svg.dataSource == kiriview::ImageDecodeDataSource::Original);

    const kiriview::ImageInputClassification heif
        = kiriview::imageInputClassificationFromBridge(kiriview::RustImageInputClassification {
            kiriview::RustImageInputKind::HeifFamily,
            kiriview::RustQtRasterFormat::None,
            kiriview::RustImageDecodeDataSource::AvifCompatible,
        });
    QVERIFY(heif.kind == kiriview::ImageInputKind::HeifFamily);
    QVERIFY(heif.qtFormat == kiriview::QtRasterFormat::None);
    QVERIFY(heif.dataSource == kiriview::ImageDecodeDataSource::AvifCompatible);

    const kiriview::ImageInputClassification jxl
        = kiriview::imageInputClassificationFromBridge(kiriview::RustImageInputClassification {
            kiriview::RustImageInputKind::QtRaster,
            kiriview::RustQtRasterFormat::Jxl,
            kiriview::RustImageDecodeDataSource::Original,
        });
    QVERIFY(jxl.kind == kiriview::ImageInputKind::QtRaster);
    QVERIFY(jxl.qtFormat == kiriview::QtRasterFormat::Jxl);
    QVERIFY(jxl.dataSource == kiriview::ImageDecodeDataSource::Original);
}

QTEST_GUILESS_MAIN(TestImageInputClassificationConversion)

#include "test_imageinputclassificationconversion.moc"
