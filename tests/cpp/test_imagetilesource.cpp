// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilesource.h"

#include <QBuffer>
#include <QByteArrayList>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QTest>
#include <Qt>
#include <memory>
#include <optional>

namespace {
QByteArray encodedImageData(const QImage &image, const QByteArray &format, QString *errorString);

QByteArray pngData()
{
    QImage image(4, 4, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    image.setPixelColor(0, 0, Qt::red);
    image.setPixelColor(3, 3, Qt::blue);

    return encodedImageData(image, QByteArrayLiteral("png"), nullptr);
}

QByteArray encodedImageData(const QImage &image, const QByteArray &format, QString *errorString)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, format);
    if (!writer.write(image)) {
        if (errorString != nullptr) {
            *errorString = writer.errorString();
        }
        return {};
    }
    return data;
}

bool imageWriterSupports(const QByteArray &format)
{
    const QByteArrayList formats = QImageWriter::supportedImageFormats();
    return formats.contains(format) || formats.contains(format.toUpper());
}

QByteArray jpegWriterFormat()
{
    return imageWriterSupports(QByteArrayLiteral("jpg")) ? QByteArrayLiteral("jpg")
                                                         : QByteArrayLiteral("jpeg");
}
}

class TestImageTileSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void qImageReaderSourceDecodesBlockingDisplayImageAndTile();
    void jpegSourceDecodesFirstDisplayToViewport();
    void jpegSourceSkipsFirstDisplayWhenImageFitsViewport();
    void pngSourceLeavesFirstDisplayNotImplemented();
    void svgSourceRendersIntrinsicPreviewAndTile();
};

void TestImageTileSource::qImageReaderSourceDecodesBlockingDisplayImageAndTile()
{
    QString errorString;
    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(pngData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(4, 4));

    const QImage preview = source->decodeBlockingDisplayImage(2, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), QSize(2, 2));

    const KiriView::TilePyramid pyramid(source->imageSize());
    const KiriView::TileRequest request = pyramid.requestForTile(KiriView::TileKey { 0, 0, 0 });
    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.size(), QSize(4, 4));
}

void TestImageTileSource::jpegSourceDecodesFirstDisplayToViewport()
{
    if (!imageWriterSupports(QByteArrayLiteral("jpg"))
        && !imageWriterSupports(QByteArrayLiteral("jpeg"))) {
        QSKIP("Qt JPEG image writer is unavailable");
    }

    QImage image(1600, 1200, QImage::Format_RGB32);
    image.fill(Qt::red);
    QString errorString;
    const QByteArray data = encodedImageData(image, jpegWriterFormat(), &errorString);
    QVERIFY2(!data.isEmpty(), qPrintable(errorString));

    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const KiriView::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    QCOMPARE(result.status, KiriView::FirstDisplayImageDecodeStatus::Ready);
    QVERIFY2(!result.image.isNull(), qPrintable(errorString));
    QCOMPARE(result.image.size(), QSize(400, 300));
    QVERIFY(result.image.size() != source->imageSize());
    QCOMPARE(result.displayPixelsPerSourcePixel, 0.25);
}

void TestImageTileSource::jpegSourceSkipsFirstDisplayWhenImageFitsViewport()
{
    if (!imageWriterSupports(QByteArrayLiteral("jpg"))
        && !imageWriterSupports(QByteArrayLiteral("jpeg"))) {
        QSKIP("Qt JPEG image writer is unavailable");
    }

    QImage image(200, 100, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QString errorString;
    const QByteArray data = encodedImageData(image, jpegWriterFormat(), &errorString);
    QVERIFY2(!data.isEmpty(), qPrintable(errorString));

    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const KiriView::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    QCOMPARE(result.status, KiriView::FirstDisplayImageDecodeStatus::NotImplemented);
    QVERIFY(result.image.isNull());
}

void TestImageTileSource::pngSourceLeavesFirstDisplayNotImplemented()
{
    QString errorString;
    std::shared_ptr<KiriView::QImageReaderTileSource> source
        = KiriView::QImageReaderTileSource::open(pngData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const KiriView::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(2, 2) }, &errorString);
    QCOMPARE(result.status, KiriView::FirstDisplayImageDecodeStatus::NotImplemented);

    const QImage blockingDisplay = source->decodeBlockingDisplayImage(2, &errorString);
    QVERIFY2(!blockingDisplay.isNull(), qPrintable(errorString));
    QCOMPARE(blockingDisplay.size(), QSize(2, 2));
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

    const KiriView::FirstDisplayImageDecodeResult firstDisplay = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(20, 20) }, &errorString);
    QCOMPARE(firstDisplay.status, KiriView::FirstDisplayImageDecodeStatus::NotImplemented);

    const QImage preview = source->decodeBlockingDisplayImage(20, &errorString);
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
