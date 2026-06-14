// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/qimagereadertilesource.h"

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

QByteArray pngData()
{
    QImage image(4, 4, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    image.setPixelColor(0, 0, Qt::red);
    image.setPixelColor(3, 3, Qt::blue);

    return encodedImageData(image, QByteArrayLiteral("png"), nullptr);
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

class TestQImageReaderTileSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceDecodesBlockingDisplayImageAndTile();
    void failedTileDecodePreservesAttemptDiagnostics();
    void jpegSourceDecodesFirstDisplayToViewport();
    void jpegSourceSkipsFirstDisplayWhenImageFitsViewport();
    void pngSourceLeavesFirstDisplayNotImplemented();
};

void TestQImageReaderTileSource::sourceDecodesBlockingDisplayImageAndTile()
{
    QString errorString;
    std::shared_ptr<kiriview::QImageReaderTileSource> source
        = kiriview::QImageReaderTileSource::open(pngData(), QByteArrayLiteral("png"), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(4, 4));

    const QImage preview = source->decodeBlockingDisplayImage(2, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), QSize(2, 2));

    const kiriview::TilePyramid pyramid(source->imageSize());
    const kiriview::TileRequest request = pyramid.requestForTile(kiriview::TileKey { 0, 0, 0 });
    const std::optional<kiriview::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.size(), QSize(4, 4));
}

void TestQImageReaderTileSource::failedTileDecodePreservesAttemptDiagnostics()
{
    kiriview::QImageReaderTileSource source(QByteArrayLiteral("not image data"),
        QByteArrayLiteral("png"), QSize(4, 4), kiriview::StaticImageReaderTransform {});

    const kiriview::TilePyramid pyramid(source.imageSize());
    const kiriview::TileRequest request = pyramid.requestForTile(kiriview::TileKey { 0, 0, 0 });
    const kiriview::QImageReaderTileDecodeResult result = source.decodeTileWithDiagnostics(request);

    QVERIFY(!result.tile.has_value());
    QCOMPARE(result.diagnostics.failures.size(), 4);
    QCOMPARE(result.diagnostics.failures.at(0).kind,
        kiriview::QImageReaderTileDecodeAttemptKind::ReaderClip);
    QCOMPARE(result.diagnostics.failures.at(1).kind,
        kiriview::QImageReaderTileDecodeAttemptKind::SourceClip);
    QCOMPARE(result.diagnostics.failures.at(2).kind,
        kiriview::QImageReaderTileDecodeAttemptKind::ScaledLevel);
    QCOMPARE(result.diagnostics.failures.at(3).kind,
        kiriview::QImageReaderTileDecodeAttemptKind::FullImageFallback);
    for (const kiriview::QImageReaderTileDecodeAttemptFailure &failure :
        result.diagnostics.failures) {
        QVERIFY(!failure.errorString.isEmpty());
    }

    QString compatibilityError;
    const std::optional<kiriview::DecodedTile> compatibilityTile
        = source.decodeTile(request, &compatibilityError);
    QVERIFY(!compatibilityTile.has_value());
    QCOMPARE(compatibilityError, result.diagnostics.userMessage());
}

void TestQImageReaderTileSource::jpegSourceDecodesFirstDisplayToViewport()
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

    std::shared_ptr<kiriview::QImageReaderTileSource> source
        = kiriview::QImageReaderTileSource::open(data, QByteArrayLiteral("jpeg"), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const kiriview::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    QCOMPARE(result.status, kiriview::FirstDisplayImageDecodeStatus::Ready);
    QVERIFY2(!result.image.isNull(), qPrintable(errorString));
    QCOMPARE(result.image.size(), QSize(400, 300));
    QVERIFY(result.image.size() != source->imageSize());
    QCOMPARE(result.displayPixelsPerSourcePixel, 0.25);
}

void TestQImageReaderTileSource::jpegSourceSkipsFirstDisplayWhenImageFitsViewport()
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

    std::shared_ptr<kiriview::QImageReaderTileSource> source
        = kiriview::QImageReaderTileSource::open(data, QByteArrayLiteral("jpeg"), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const kiriview::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    QCOMPARE(result.status, kiriview::FirstDisplayImageDecodeStatus::NotImplemented);
    QVERIFY(result.image.isNull());
}

void TestQImageReaderTileSource::pngSourceLeavesFirstDisplayNotImplemented()
{
    QString errorString;
    std::shared_ptr<kiriview::QImageReaderTileSource> source
        = kiriview::QImageReaderTileSource::open(pngData(), QByteArrayLiteral("png"), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const kiriview::FirstDisplayImageDecodeResult result = source->decodeFirstDisplayImage(
        kiriview::ImageFirstDisplayDecodeContext { QSize(2, 2) }, &errorString);
    QCOMPARE(result.status, kiriview::FirstDisplayImageDecodeStatus::NotImplemented);

    const QImage blockingDisplay = source->decodeBlockingDisplayImage(2, &errorString);
    QVERIFY2(!blockingDisplay.isNull(), qPrintable(errorString));
    QCOMPARE(blockingDisplay.size(), QSize(2, 2));
}

QTEST_GUILESS_MAIN(TestQImageReaderTileSource)

#include "test_qimagereadertilesource.moc"
