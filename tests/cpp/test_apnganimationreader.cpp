// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/apnganimationreader.h"
#include "decoding/decodedimageresult.h"
#include "decoding/kiriimagedecoder.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QIODevice>
#include <QImage>
#include <QObject>
#include <QTest>
#include <QtGlobal>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
constexpr std::array<unsigned char, 8> pngSignature { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };

struct FrameSpec {
    quint32 width = 0;
    quint32 height = 0;
    quint32 xOffset = 0;
    quint32 yOffset = 0;
    quint16 delayNum = 1;
    quint16 delayDen = 10;
    unsigned char disposeOp = 0;
    unsigned char blendOp = 0;
    QByteArray pixels;
};

QByteArray pixelBytes(std::initializer_list<std::array<unsigned char, 4>> pixels)
{
    QByteArray bytes;
    bytes.reserve(static_cast<qsizetype>(pixels.size() * 4));
    for (const std::array<unsigned char, 4> &pixel : pixels) {
        bytes.append(reinterpret_cast<const char *>(pixel.data()), 4);
    }
    return bytes;
}

FrameSpec fullCanvasFrame(quint32 width, quint32 height, QByteArray pixels)
{
    return FrameSpec { width, height, 0, 0, 1, 10, 0, 0, std::move(pixels) };
}

quint32 readBe32(const QByteArray &data, qsizetype offset)
{
    return (static_cast<quint32>(static_cast<unsigned char>(data[offset])) << 24)
        | (static_cast<quint32>(static_cast<unsigned char>(data[offset + 1])) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data[offset + 2])) << 8)
        | static_cast<quint32>(static_cast<unsigned char>(data[offset + 3]));
}

void appendBe32(QByteArray *data, quint32 value)
{
    data->append(static_cast<char>((value >> 24) & 0xff));
    data->append(static_cast<char>((value >> 16) & 0xff));
    data->append(static_cast<char>((value >> 8) & 0xff));
    data->append(static_cast<char>(value & 0xff));
}

void appendBe16(QByteArray *data, quint16 value)
{
    data->append(static_cast<char>((value >> 8) & 0xff));
    data->append(static_cast<char>(value & 0xff));
}

quint32 crc32(const QByteArray &data)
{
    quint32 crc = 0xffffffffU;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

void appendPngChunk(QByteArray *png, const char *kind, const QByteArray &payload)
{
    appendBe32(png, static_cast<quint32>(payload.size()));
    const qsizetype typeOffset = png->size();
    png->append(kind, 4);
    png->append(payload);
    appendBe32(png, crc32(png->mid(typeOffset, 4 + payload.size())));
}

std::vector<QByteArray> extractChunks(const QByteArray &png, const char *expectedKind)
{
    std::vector<QByteArray> chunks;
    qsizetype offset = static_cast<qsizetype>(pngSignature.size());
    while (offset + 12 <= png.size()) {
        const quint32 length = readBe32(png, offset);
        const qsizetype payloadOffset = offset + 8;
        const qsizetype nextOffset = payloadOffset + static_cast<qsizetype>(length) + 4;
        if (nextOffset > png.size()) {
            break;
        }
        if (std::memcmp(png.constData() + offset + 4, expectedKind, 4) == 0) {
            chunks.push_back(png.mid(payloadOffset, static_cast<qsizetype>(length)));
        }
        offset = nextOffset;
    }
    return chunks;
}

QByteArray firstChunk(const QByteArray &png, const char *kind)
{
    const std::vector<QByteArray> chunks = extractChunks(png, kind);
    Q_ASSERT(!chunks.empty());
    return chunks.front();
}

QByteArray encodeRgbaPng(quint32 width, quint32 height, const QByteArray &pixels)
{
    QImage image(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGBA8888);
    Q_ASSERT(!image.isNull());
    for (int y = 0; y < image.height(); ++y) {
        std::memcpy(image.scanLine(y), pixels.constData() + y * image.width() * 4,
            static_cast<size_t>(image.width() * 4));
    }

    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    Q_ASSERT(image.save(&buffer, "PNG"));
    return png;
}

QByteArray frameControlPayload(quint32 sequenceNumber, const FrameSpec &frame)
{
    QByteArray payload;
    appendBe32(&payload, sequenceNumber);
    appendBe32(&payload, frame.width);
    appendBe32(&payload, frame.height);
    appendBe32(&payload, frame.xOffset);
    appendBe32(&payload, frame.yOffset);
    appendBe16(&payload, frame.delayNum);
    appendBe16(&payload, frame.delayDen);
    payload.append(static_cast<char>(frame.disposeOp));
    payload.append(static_cast<char>(frame.blendOp));
    return payload;
}

QByteArray makeApng(quint32 canvasWidth, quint32 canvasHeight, quint32 playCount,
    const std::vector<FrameSpec> &frames, std::optional<QByteArray> hiddenDefault = std::nullopt)
{
    Q_ASSERT(!frames.empty());
    const QByteArray defaultPixels = hiddenDefault.value_or(frames.front().pixels);
    const QByteArray defaultPng = encodeRgbaPng(canvasWidth, canvasHeight, defaultPixels);

    QByteArray apng;
    apng.append(reinterpret_cast<const char *>(pngSignature.data()), pngSignature.size());
    appendPngChunk(&apng, "IHDR", firstChunk(defaultPng, "IHDR"));

    QByteArray animationControl;
    appendBe32(&animationControl, static_cast<quint32>(frames.size()));
    appendBe32(&animationControl, playCount);
    appendPngChunk(&apng, "acTL", animationControl);

    quint32 sequenceNumber = 0;
    if (!hiddenDefault.has_value()) {
        appendPngChunk(&apng, "fcTL", frameControlPayload(sequenceNumber++, frames.front()));
        for (const QByteArray &idat : extractChunks(defaultPng, "IDAT")) {
            appendPngChunk(&apng, "IDAT", idat);
        }
    } else {
        for (const QByteArray &idat : extractChunks(defaultPng, "IDAT")) {
            appendPngChunk(&apng, "IDAT", idat);
        }
    }

    const qsizetype firstFdatFrame = hiddenDefault.has_value() ? 0 : 1;
    for (qsizetype index = firstFdatFrame; index < static_cast<qsizetype>(frames.size()); ++index) {
        const FrameSpec &frame = frames[static_cast<std::size_t>(index)];
        appendPngChunk(&apng, "fcTL", frameControlPayload(sequenceNumber++, frame));
        const QByteArray framePng = encodeRgbaPng(frame.width, frame.height, frame.pixels);
        QByteArray frameData;
        appendBe32(&frameData, sequenceNumber++);
        for (const QByteArray &idat : extractChunks(framePng, "IDAT")) {
            frameData.append(idat);
        }
        appendPngChunk(&apng, "fdAT", frameData);
    }

    appendPngChunk(&apng, "IEND", {});
    return apng;
}

QColor pixel(const QImage &image, int x, int y) { return image.pixelColor(x, y); }

template <typename Image> const Image *decodedImage(const kiriview::DecodedImageResult &result)
{
    const kiriview::DecodedImage *image = kiriview::decodedImageResultImage(result);
    return image == nullptr ? nullptr : std::get_if<Image>(image);
}
}

class TestApngAnimationReader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nonPngAndPlainPngReturnNotApng();
    void readerDecodesSequentialFramesAndLoopCount();
    void hiddenDefaultImageIsNotDisplayed();
    void blendOverComposesWithExistingCanvas();
    void disposeBackgroundClearsFrameRegion();
    void disposePreviousRestoresFrameRegion();
    void malformedApngReportsError();
    void imageDecoderReturnsStreamingApngImage();
};

void TestApngAnimationReader::nonPngAndPlainPngReturnNotApng()
{
    kiriview::ApngAnimationReader reader;
    kiriview::ApngOpenResult result = reader.open(QByteArrayLiteral("not png"));
    QCOMPARE(result.status, kiriview::ApngOpenStatus::NotApng);

    const QByteArray png = encodeRgbaPng(1, 1, pixelBytes({ { { 255, 0, 0, 255 } } }));
    result = reader.open(png);
    QCOMPARE(result.status, kiriview::ApngOpenStatus::NotApng);
}

void TestApngAnimationReader::readerDecodesSequentialFramesAndLoopCount()
{
    FrameSpec first = fullCanvasFrame(1, 1, pixelBytes({ { { 255, 0, 0, 255 } } }));
    FrameSpec second = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 255 } } }));
    second.delayNum = 2;
    const QByteArray apng = makeApng(1, 1, 2, { first, second });

    kiriview::ApngAnimationReader reader;
    const kiriview::ApngOpenResult result = reader.open(apng);
    QCOMPARE(result.status, kiriview::ApngOpenStatus::Success);
    QCOMPARE(result.frameCount, 2);
    QCOMPARE(result.loopCount, 1);
    QCOMPARE(result.firstFrameDelay, 100);
    QCOMPARE(pixel(result.firstFrame, 0, 0), QColor(255, 0, 0, 255));
    QVERIFY(reader.hasMoreFrames());

    QString errorString;
    const std::optional<kiriview::AnimationFrame> frame = reader.readNextFrame(&errorString);
    QVERIFY2(frame.has_value(), qPrintable(errorString));
    QCOMPARE(frame->delay, 200);
    QCOMPARE(pixel(frame->image, 0, 0), QColor(0, 0, 255, 255));
    QVERIFY(!reader.hasMoreFrames());
    QVERIFY(!reader.readNextFrame(&errorString).has_value());
    QVERIFY(errorString.isEmpty());
}

void TestApngAnimationReader::hiddenDefaultImageIsNotDisplayed()
{
    FrameSpec animationFrame = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 255 } } }));
    const QByteArray hiddenDefault = pixelBytes({ { { 255, 0, 0, 255 } } });
    const QByteArray apng = makeApng(1, 1, 0, { animationFrame }, hiddenDefault);

    kiriview::ApngAnimationReader reader;
    const kiriview::ApngOpenResult result = reader.open(apng);
    QCOMPARE(result.status, kiriview::ApngOpenStatus::Success);
    QCOMPARE(result.frameCount, 1);
    QCOMPARE(result.loopCount, -1);
    QCOMPARE(pixel(result.firstFrame, 0, 0), QColor(0, 0, 255, 255));
    QVERIFY(!reader.hasMoreFrames());
}

void TestApngAnimationReader::blendOverComposesWithExistingCanvas()
{
    FrameSpec first = fullCanvasFrame(1, 1, pixelBytes({ { { 255, 0, 0, 255 } } }));
    FrameSpec second = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 128 } } }));
    second.blendOp = 1;
    const QByteArray apng = makeApng(1, 1, 1, { first, second });

    kiriview::ApngAnimationReader reader;
    QCOMPARE(reader.open(apng).status, kiriview::ApngOpenStatus::Success);

    QString errorString;
    const std::optional<kiriview::AnimationFrame> frame = reader.readNextFrame(&errorString);
    QVERIFY2(frame.has_value(), qPrintable(errorString));
    const QColor color = pixel(frame->image, 0, 0);
    QCOMPARE(color.alpha(), 255);
    QVERIFY(color.red() > 0);
    QVERIFY(color.blue() > 0);
}

void TestApngAnimationReader::disposeBackgroundClearsFrameRegion()
{
    FrameSpec first
        = fullCanvasFrame(2, 1, pixelBytes({ { { 255, 0, 0, 255 } }, { { 255, 0, 0, 255 } } }));
    FrameSpec second = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 255 } } }));
    second.disposeOp = 1;
    FrameSpec third = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 255, 0, 255 } } }));
    third.xOffset = 1;
    const QByteArray apng = makeApng(2, 1, 1, { first, second, third });

    kiriview::ApngAnimationReader reader;
    QCOMPARE(reader.open(apng).status, kiriview::ApngOpenStatus::Success);

    QString errorString;
    QVERIFY(reader.readNextFrame(&errorString).has_value());
    const std::optional<kiriview::AnimationFrame> thirdFrame = reader.readNextFrame(&errorString);
    QVERIFY2(thirdFrame.has_value(), qPrintable(errorString));
    QCOMPARE(pixel(thirdFrame->image, 0, 0).alpha(), 0);
    QCOMPARE(pixel(thirdFrame->image, 1, 0), QColor(0, 255, 0, 255));
}

void TestApngAnimationReader::disposePreviousRestoresFrameRegion()
{
    FrameSpec first
        = fullCanvasFrame(2, 1, pixelBytes({ { { 255, 0, 0, 255 } }, { { 255, 0, 0, 255 } } }));
    FrameSpec second = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 255 } } }));
    second.disposeOp = 2;
    FrameSpec third = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 255, 0, 255 } } }));
    third.xOffset = 1;
    const QByteArray apng = makeApng(2, 1, 1, { first, second, third });

    kiriview::ApngAnimationReader reader;
    QCOMPARE(reader.open(apng).status, kiriview::ApngOpenStatus::Success);

    QString errorString;
    QVERIFY(reader.readNextFrame(&errorString).has_value());
    const std::optional<kiriview::AnimationFrame> thirdFrame = reader.readNextFrame(&errorString);
    QVERIFY2(thirdFrame.has_value(), qPrintable(errorString));
    QCOMPARE(pixel(thirdFrame->image, 0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(pixel(thirdFrame->image, 1, 0), QColor(0, 255, 0, 255));
}

void TestApngAnimationReader::malformedApngReportsError()
{
    FrameSpec frame
        = fullCanvasFrame(2, 1, pixelBytes({ { { 255, 0, 0, 255 } }, { { 255, 0, 0, 255 } } }));
    const QByteArray apng = makeApng(1, 1, 1, { frame });

    kiriview::ApngAnimationReader reader;
    const kiriview::ApngOpenResult result = reader.open(apng);
    QCOMPARE(result.status, kiriview::ApngOpenStatus::Error);
    QVERIFY(result.errorString.contains(QStringLiteral("APNG")));
}

void TestApngAnimationReader::imageDecoderReturnsStreamingApngImage()
{
    FrameSpec first = fullCanvasFrame(1, 1, pixelBytes({ { { 255, 0, 0, 255 } } }));
    FrameSpec second = fullCanvasFrame(1, 1, pixelBytes({ { { 0, 0, 255, 255 } } }));
    const QByteArray apng = makeApng(1, 1, 1, { first, second });

    const kiriview::DecodedImageResult result = kiriview::decodeImageData(apng);
    const auto *decoded = decodedImage<kiriview::ApngAnimationImage>(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(pixel(decoded->firstFrame, 0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(decoded->data, apng);
}

QTEST_GUILESS_MAIN(TestApngAnimationReader)

#include "test_apnganimationreader.moc"
