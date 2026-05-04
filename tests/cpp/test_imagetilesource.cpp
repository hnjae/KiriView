// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include <QBuffer>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QTest>
#include <Qt>
#include <memory>
#include <optional>

namespace {
QByteArray pngData()
{
    QImage image(4, 4, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    image.setPixelColor(0, 0, Qt::red);
    image.setPixelColor(3, 3, Qt::blue);

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "png");
    writer.write(image);
    return data;
}
}

class TestImageTileSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void qImageReaderSourceDecodesPreviewAndTile();
    void svgSourceRendersIntrinsicPreviewAndTile();
};

void TestImageTileSource::qImageReaderSourceDecodesPreviewAndTile()
{
    QString errorString;
    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(pngData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(4, 4));

    const QImage preview = source->decodePreview(2, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), QSize(2, 2));

    const KiriView::TilePyramid pyramid(source->imageSize());
    const KiriView::TileRequest request = pyramid.requestForTile(KiriView::TileKey { 0, 0, 0 });
    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.size(), QSize(4, 4));
}

void TestImageTileSource::svgSourceRendersIntrinsicPreviewAndTile()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(80, 40));

    const QImage preview = source->decodePreview(20, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), QSize(20, 10));

    const KiriView::TilePyramid pyramid(source->imageSize());
    const KiriView::TileRequest request = pyramid.requestForTile(KiriView::TileKey { 0, 0, 0 });
    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.size(), QSize(80, 40));
    QVERIFY(qRed(tile->image.pixel(10, 10)) > 0);
}

QTEST_GUILESS_MAIN(TestImageTileSource)

#include "test_imagetilesource.moc"
