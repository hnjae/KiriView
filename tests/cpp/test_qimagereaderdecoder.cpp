// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "qimagereaderdecoder.h"

#include "rendering/qimagereadertilesource.h"

#include <QBuffer>
#include <QByteArrayList>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <Qt>

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

template <typename Image> const Image *decodedImage(const KiriView::DecodedImageResult &result)
{
    return KiriView::decodedImageResultImageAs<Image>(result);
}
}

class TestQImageReaderDecoder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void invalidDataReturnsFailure();
    void pngDataDecodesAsStaticTileSource();
    void jpegDataUsesFirstDisplayRequest();
};

void TestQImageReaderDecoder::invalidDataReturnsFailure()
{
    const KiriView::DecodedImageResult result = KiriView::decodeQImageReaderImageData(
        QByteArrayLiteral("not image data"), {}, KiriView::QtRasterFormat::Png);

    const KiriView::DecodedImageFailure *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QVERIFY(!failure->errorString.isEmpty());
    QVERIFY(decodedImage<KiriView::StaticDecodedImage>(result) == nullptr);
}

void TestQImageReaderDecoder::pngDataDecodesAsStaticTileSource()
{
    QImage image(4, 4, QImage::Format_RGBA8888);
    image.fill(Qt::red);

    QString errorString;
    const QByteArray data = encodedImageData(image, QByteArrayLiteral("png"), &errorString);
    QVERIFY2(!data.isEmpty(), qPrintable(errorString));

    const KiriView::DecodedImageResult result
        = KiriView::decodeQImageReaderImageData(data, {}, KiriView::QtRasterFormat::Png);
    const KiriView::StaticDecodedImage *decoded
        = decodedImage<KiriView::StaticDecodedImage>(result);

    QVERIFY(decoded != nullptr);
    QVERIFY(dynamic_cast<KiriView::QImageReaderTileSource *>(decoded->staticImage.source.get())
        != nullptr);
    QCOMPARE(decoded->staticImage.source->imageSize(), QSize(4, 4));
    QCOMPARE(decoded->staticImage.preview.size(), QSize(4, 4));
    QCOMPARE(decoded->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.0);
}

void TestQImageReaderDecoder::jpegDataUsesFirstDisplayRequest()
{
    if (!imageWriterSupports(QByteArrayLiteral("jpg"))
        && !imageWriterSupports(QByteArrayLiteral("jpeg"))) {
        QSKIP("Qt JPEG image writer is unavailable");
    }

    QImage image(1600, 1200, QImage::Format_RGB32);
    image.fill(Qt::blue);

    QString errorString;
    const QByteArray data = encodedImageData(image, jpegWriterFormat(), &errorString);
    QVERIFY2(!data.isEmpty(), qPrintable(errorString));

    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(1,
        QUrl::fromLocalFile(QStringLiteral("/tmp/photo.jpg")),
        KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) });
    const KiriView::DecodedImageResult result
        = KiriView::decodeQImageReaderImageData(data, request, KiriView::QtRasterFormat::Jpeg);
    const KiriView::StaticDecodedImage *decoded
        = decodedImage<KiriView::StaticDecodedImage>(result);

    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->staticImage.source->imageSize(), QSize(1600, 1200));
    QCOMPARE(decoded->staticImage.preview.size(), QSize(400, 300));
    QCOMPARE(decoded->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.25);
}

QTEST_GUILESS_MAIN(TestQImageReaderDecoder)

#include "test_qimagereaderdecoder.moc"
