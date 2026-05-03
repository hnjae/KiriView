// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagedecoder.h"

#include <libheif/heif.h>

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QSize>
#include <QTest>
#include <QtGlobal>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>

namespace {
struct HeifLibraryScope {
    explicit HeifLibraryScope(QString *error)
    {
        const heif_error initError = heif_init(nullptr);
        if (initError.code != heif_error_Ok) {
            *error = QString::fromUtf8(initError.message != nullptr ? initError.message : "");
            return;
        }
        initialized = true;
    }

    ~HeifLibraryScope()
    {
        if (initialized) {
            heif_deinit();
        }
    }

    bool initialized = false;
};

struct MemoryWriter {
    QByteArray data;
};

heif_error writeHeifBytes(heif_context *, const void *data, size_t size, void *userdata)
{
    heif_error error { heif_error_Ok, heif_suberror_Unspecified, "Success" };
    if (data == nullptr || userdata == nullptr) {
        error.code = heif_error_Usage_error;
        error.subcode = heif_suberror_Null_pointer_argument;
        error.message = "Null writer argument";
        return error;
    }
    if (size > static_cast<size_t>(std::numeric_limits<qsizetype>::max())) {
        error.code = heif_error_Encoding_error;
        error.subcode = heif_suberror_Cannot_write_output_data;
        error.message = "Output chunk is too large";
        return error;
    }

    auto *writer = static_cast<MemoryWriter *>(userdata);
    writer->data.append(static_cast<const char *>(data), static_cast<qsizetype>(size));
    return error;
}

QString heifErrorText(const char *action, const heif_error &error)
{
    return QString::fromLatin1(action) + QStringLiteral(": ")
        + QString::fromUtf8(error.message != nullptr ? error.message : "");
}

std::optional<QByteArray> createJpegCompressedHeifData(QString *errorText)
{
    HeifLibraryScope library(errorText);
    if (!library.initialized) {
        return std::nullopt;
    }

    std::unique_ptr<heif_context, decltype(&heif_context_free)> context(
        heif_context_alloc(), heif_context_free);
    if (context == nullptr) {
        *errorText = QStringLiteral("heif_context_alloc failed");
        return std::nullopt;
    }

    heif_image *rawImage = nullptr;
    heif_error error
        = heif_image_create(2, 2, heif_colorspace_RGB, heif_chroma_interleaved_RGB, &rawImage);
    if (error.code != heif_error_Ok) {
        *errorText = heifErrorText("heif_image_create", error);
        return std::nullopt;
    }
    std::unique_ptr<heif_image, decltype(&heif_image_release)> image(rawImage, heif_image_release);

    error = heif_image_add_plane(image.get(), heif_channel_interleaved, 2, 2, 8);
    if (error.code != heif_error_Ok) {
        *errorText = heifErrorText("heif_image_add_plane", error);
        return std::nullopt;
    }

    int stride = 0;
    uint8_t *plane = heif_image_get_plane(image.get(), heif_channel_interleaved, &stride);
    if (plane == nullptr || stride < 6) {
        *errorText = QStringLiteral("heif_image_get_plane returned invalid data");
        return std::nullopt;
    }

    const std::array<uint8_t, 12> pixels {
        255,
        0,
        0,
        0,
        255,
        0,
        0,
        0,
        255,
        255,
        255,
        255,
    };
    std::memcpy(plane, pixels.data(), 6);
    std::memcpy(plane + stride, pixels.data() + 6, 6);

    heif_encoder *rawEncoder = nullptr;
    error = heif_context_get_encoder_for_format(context.get(), heif_compression_JPEG, &rawEncoder);
    if (error.code != heif_error_Ok) {
        *errorText = heifErrorText("heif_context_get_encoder_for_format", error);
        return std::nullopt;
    }
    std::unique_ptr<heif_encoder, decltype(&heif_encoder_release)> encoder(
        rawEncoder, heif_encoder_release);
    heif_encoder_set_lossy_quality(encoder.get(), 90);

    heif_image_handle *rawHandle = nullptr;
    error
        = heif_context_encode_image(context.get(), image.get(), encoder.get(), nullptr, &rawHandle);
    if (error.code != heif_error_Ok) {
        *errorText = heifErrorText("heif_context_encode_image", error);
        return std::nullopt;
    }
    std::unique_ptr<heif_image_handle, decltype(&heif_image_handle_release)> handle(
        rawHandle, heif_image_handle_release);

    MemoryWriter memoryWriter;
    heif_writer writer {};
    writer.writer_api_version = 1;
    writer.write = writeHeifBytes;
    error = heif_context_write(context.get(), &writer, &memoryWriter);
    if (error.code != heif_error_Ok) {
        *errorText = heifErrorText("heif_context_write", error);
        return std::nullopt;
    }

    if (memoryWriter.data.size() < 16 || memoryWriter.data.mid(4, 4) != QByteArrayLiteral("ftyp")) {
        *errorText = QStringLiteral("encoded HEIF data does not start with an ftyp box");
        return std::nullopt;
    }

    const int jpegBrandOffset = memoryWriter.data.indexOf(QByteArrayLiteral("jpeg"), 16);
    if (jpegBrandOffset < 0) {
        *errorText = QStringLiteral("encoded HEIF data does not advertise the jpeg brand");
        return std::nullopt;
    }
    std::memcpy(memoryWriter.data.data() + 8, "jpeg", 4);

    return memoryWriter.data;
}
}

class TestKiriImageDecoder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void jpegCompressedHeifStillImageDecodes();
    void heifSequenceDecodesAsStreamingAnimation();
};

void TestKiriImageDecoder::jpegCompressedHeifStillImageDecodes()
{
    QString encodeError;
    const std::optional<QByteArray> imageData = createJpegCompressedHeifData(&encodeError);
    QVERIFY2(imageData.has_value(), qPrintable(encodeError));
    QCOMPARE(imageData->mid(4, 4), QByteArrayLiteral("ftyp"));
    QCOMPARE(imageData->mid(8, 4), QByteArrayLiteral("jpeg"));

    KiriView::DecodedImageResult result = KiriView::decodeImageData(*imageData);
    const auto *decoded = std::get_if<KiriView::StaticDecodedImage>(&result);
    QVERIFY2(decoded != nullptr, "JPEG-compressed HEIF should decode as a static image");
    QCOMPARE(decoded->image.size(), QSize(2, 2));
    QVERIFY(!decoded->image.isNull());
}

void TestKiriImageDecoder::heifSequenceDecodesAsStreamingAnimation()
{
    QFile file(QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/heif-sequence-alpha.heics"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));
    const QByteArray imageData = file.readAll();
    QVERIFY(!imageData.isEmpty());

    KiriView::DecodedImageResult result = KiriView::decodeImageData(imageData);
    const auto *decoded = std::get_if<KiriView::HeifSequenceAnimationImage>(&result);
    QVERIFY2(decoded != nullptr, "HEIF image sequences should decode as streaming animations");
    QCOMPARE(decoded->firstFrame.size(), QSize(64, 64));
    QVERIFY(!decoded->data.isEmpty());
    QVERIFY(decoded->firstFrameDelay > 0);
    QVERIFY(qAlpha(decoded->firstFrame.pixel(48, 32)) < 255);

    KiriView::HeifSequenceReader reader;
    const KiriView::HeifSequenceOpenResult openResult = reader.open(imageData);
    QCOMPARE(openResult.status, KiriView::HeifSequenceOpenStatus::Success);

    QString errorString;
    const std::optional<KiriView::AnimationFrame> firstFrame = reader.readNextFrame(&errorString);
    QVERIFY2(firstFrame.has_value(), qPrintable(errorString));
    QCOMPARE(firstFrame->image.size(), QSize(64, 64));
    QVERIFY(firstFrame->delay > 0);
    QVERIFY(qAlpha(firstFrame->image.pixel(16, 32)) > 0);
    QVERIFY(qAlpha(firstFrame->image.pixel(48, 32)) < 255);

    const std::optional<KiriView::AnimationFrame> secondFrame = reader.readNextFrame(&errorString);
    QVERIFY2(secondFrame.has_value(), qPrintable(errorString));
    QCOMPARE(secondFrame->image.size(), QSize(64, 64));
    QVERIFY(secondFrame->delay > 0);
    QVERIFY(qAlpha(secondFrame->image.pixel(16, 32)) < 255);
    QVERIFY(qAlpha(secondFrame->image.pixel(48, 32)) > 0);
}

QTEST_GUILESS_MAIN(TestKiriImageDecoder)

#include "test_kiriimagedecoder.moc"
