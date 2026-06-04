// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/qimagereaderdecoder.h"

#include "rendering/qimagereadertilesource.h"

#include <QBuffer>
#include <QByteArrayList>
#include <QImage>
#include <QImageIOHandler>
#include <QImageWriter>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <Qt>
#include <utility>

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

void appendLeU16(QByteArray &data, quint16 value)
{
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
}

void appendLeU32(QByteArray &data, quint32 value)
{
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
    data.append(static_cast<char>((value >> 16) & 0xff));
    data.append(static_cast<char>((value >> 24) & 0xff));
}

void appendBeU16(QByteArray &data, quint16 value)
{
    data.append(static_cast<char>((value >> 8) & 0xff));
    data.append(static_cast<char>(value & 0xff));
}

QByteArray exifOrientationApp1Segment(quint16 orientation)
{
    QByteArray payload;
    payload.append("Exif\0\0", 6);
    payload.append("II", 2);
    appendLeU16(payload, 42);
    appendLeU32(payload, 8);
    appendLeU16(payload, 1);
    appendLeU16(payload, 0x0112);
    appendLeU16(payload, 3);
    appendLeU32(payload, 1);
    appendLeU16(payload, orientation);
    appendLeU16(payload, 0);
    appendLeU32(payload, 0);

    QByteArray segment;
    segment.append(char(0xff));
    segment.append(char(0xe1));
    appendBeU16(segment, static_cast<quint16>(payload.size() + 2));
    segment.append(payload);
    return segment;
}

QByteArray withExifOrientation(QByteArray jpegData, quint16 orientation)
{
    if (jpegData.size() < 2 || uchar(jpegData.at(0)) != 0xff || uchar(jpegData.at(1)) != 0xd8) {
        return {};
    }
    jpegData.insert(2, exifOrientationApp1Segment(orientation));
    return jpegData;
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
    void jpegExifOrientationProducesDisplayOrientedPayload();
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
    QVERIFY(dynamic_cast<KiriView::QImageReaderTileSource *>(
                decoded->displayImage.refinementSource.get())
        != nullptr);
    QCOMPARE(decoded->displayImage.originalSize, QSize(4, 4));
    QCOMPARE(decoded->displayImage.image.size(), QSize(4, 4));
    QCOMPARE(decoded->displayImage.quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(decoded->displayImage.displayPixelsPerSourcePixel, 1.0);
    const KiriView::StaticImagePayload compatibility
        = decoded->displayImage.compatibilityStaticImage();
    QCOMPARE(compatibility.preview.size(), QSize(4, 4));
    QCOMPARE(compatibility.displayHints.firstDisplayPixelsPerSourcePixel, 1.0);
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
    QCOMPARE(decoded->displayImage.originalSize, QSize(1600, 1200));
    QCOMPARE(decoded->displayImage.image.size(), QSize(400, 300));
    QCOMPARE(decoded->displayImage.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(decoded->displayImage.displayPixelsPerSourcePixel, 0.25);
    QVERIFY(dynamic_cast<KiriView::QImageReaderTileSource *>(
                decoded->displayImage.refinementSource.get())
        != nullptr);
}

void TestQImageReaderDecoder::jpegExifOrientationProducesDisplayOrientedPayload()
{
    if (!imageWriterSupports(QByteArrayLiteral("jpg"))
        && !imageWriterSupports(QByteArrayLiteral("jpeg"))) {
        QSKIP("Qt JPEG image writer is unavailable");
    }

    QImage image(20, 10, QImage::Format_RGB32);
    image.fill(Qt::green);

    QString errorString;
    QByteArray data = encodedImageData(image, jpegWriterFormat(), &errorString);
    QVERIFY2(!data.isEmpty(), qPrintable(errorString));
    data = withExifOrientation(std::move(data), 6);
    QVERIFY(!data.isEmpty());

    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(
        2, QUrl::fromLocalFile(QStringLiteral("/tmp/oriented.jpg")));
    const KiriView::DecodedImageResult result
        = KiriView::decodeQImageReaderImageData(data, request, KiriView::QtRasterFormat::Jpeg);
    const KiriView::StaticDecodedImage *decoded
        = decodedImage<KiriView::StaticDecodedImage>(result);

    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->displayImage.originalSize, QSize(10, 20));
    QCOMPARE(decoded->displayImage.image.size(), QSize(10, 20));
    QVERIFY(decoded->displayImage.imageReaderTransform.transformations
        != QImageIOHandler::TransformationNone);
    QVERIFY(decoded->displayImage.imageReaderTransform.transformations
        & QImageIOHandler::TransformationRotate90);
    QCOMPARE(decoded->displayImage.quality, KiriView::DisplayImageQuality::Exact);
}

QTEST_GUILESS_MAIN(TestQImageReaderDecoder)

#include "test_qimagereaderdecoder.moc"
