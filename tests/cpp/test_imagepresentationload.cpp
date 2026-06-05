// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "image_test_support.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "rendering/displayimagestore.h"
#include "rendering/imagerendering.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QIODevice>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticDecodedTestImage;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::testImage;

constexpr qsizetype testPredecodeCacheByteBudget = 1024 * 1024;
constexpr qsizetype testStaticTileCacheByteBudget = 512 * 1024;
constexpr std::array<unsigned char, 8> pngSignature { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };

struct FrameSpec {
    quint32 width = 0;
    quint32 height = 0;
    quint16 delayNum = 1;
    quint16 delayDen = 1000;
    QByteArray pixels;
};

QByteArray pixelBytes(const QColor &color)
{
    const std::array<unsigned char, 4> pixel {
        static_cast<unsigned char>(color.red()),
        static_cast<unsigned char>(color.green()),
        static_cast<unsigned char>(color.blue()),
        static_cast<unsigned char>(color.alpha()),
    };
    return QByteArray(
        reinterpret_cast<const char *>(pixel.data()), static_cast<qsizetype>(pixel.size()));
}

QImage solidRgbaImage(const QColor &color)
{
    QImage image(1, 1, QImage::Format_RGBA8888);
    std::memcpy(image.scanLine(0), pixelBytes(color).constData(), 4);
    return image;
}

FrameSpec fullCanvasFrame(const QColor &color)
{
    return FrameSpec { 1, 1, 1, 1000, pixelBytes(color) };
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
    std::memcpy(image.scanLine(0), pixels.constData(), static_cast<size_t>(width * 4));

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
    appendBe32(&payload, 0);
    appendBe32(&payload, 0);
    appendBe16(&payload, frame.delayNum);
    appendBe16(&payload, frame.delayDen);
    payload.append(static_cast<char>(0));
    payload.append(static_cast<char>(0));
    return payload;
}

QByteArray makeApng(const std::vector<FrameSpec> &frames)
{
    Q_ASSERT(!frames.empty());
    const QByteArray defaultPng = encodeRgbaPng(1, 1, frames.front().pixels);

    QByteArray apng;
    apng.append(reinterpret_cast<const char *>(pngSignature.data()), pngSignature.size());
    appendPngChunk(&apng, "IHDR", firstChunk(defaultPng, "IHDR"));

    QByteArray animationControl;
    appendBe32(&animationControl, static_cast<quint32>(frames.size()));
    appendBe32(&animationControl, 1);
    appendPngChunk(&apng, "acTL", animationControl);

    quint32 sequenceNumber = 0;
    appendPngChunk(&apng, "fcTL", frameControlPayload(sequenceNumber++, frames.front()));
    for (const QByteArray &idat : extractChunks(defaultPng, "IDAT")) {
        appendPngChunk(&apng, "IDAT", idat);
    }

    for (std::size_t index = 1; index < frames.size(); ++index) {
        const FrameSpec &frame = frames.at(index);
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

template <typename Image, typename = void> struct HasSourceIdentity : std::false_type {
};

template <typename Image>
struct HasSourceIdentity<Image, std::void_t<decltype(std::declval<Image &>().sourceIdentity)>>
    : std::true_type {
};

template <typename Image> void assignSourceIdentity(Image &image, const QString &sourceIdentity)
{
    if constexpr (HasSourceIdentity<Image>::value) {
        image.sourceIdentity = sourceIdentity;
    }
}

void assignDecodedSourceIdentity(KiriView::DecodedImage &decoded, const QString &sourceIdentity)
{
    std::visit(
        [&sourceIdentity](auto &image) { assignSourceIdentity(image, sourceIdentity); }, decoded);
}

KiriView::ImageLoadSession loadSession(const QUrl &url)
{
    return KiriView::ImageLoadSession(1, KiriView::ImageLoadRequest::fromUrl(url),
        KiriView::DisplayedImageLocation::fromUrl(url));
}

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}

KiriView::ImagePageSurfaceController pageSurfaceController(QObject *parent)
{
    return KiriView::ImagePageSurfaceController(parent, {},
        KiriView::ImageCacheBudgets {
            testPredecodeCacheByteBudget,
            testStaticTileCacheByteBudget,
        });
}

KiriView::ImagePageSurfaceController pageSurfaceController(
    QObject *parent, std::shared_ptr<KiriView::DisplayImageStore> displayImageStore)
{
    return KiriView::ImagePageSurfaceController(parent, {},
        KiriView::ImageCacheBudgets {
            testPredecodeCacheByteBudget,
            testStaticTileCacheByteBudget,
        },
        std::move(displayImageStore));
}

QString entryId(const KiriView::ImageDisplaySourceSlot &slot)
{
    return slot.providerUrl.path().mid(1);
}

template <typename Load> const Load *planPayload(const KiriView::ImagePresentationLoadPlan &plan)
{
    return std::get_if<Load>(&plan.payload);
}
}

class TestImagePresentationLoad : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void predecodedImagesPlanStaticCacheablePresentation();
    void predecodedImagesPublishProviderSourceAndKeepCompatibilitySurface();
    void decodedImagesPlanPresentationActions();
    void staticDecodedPredecodeCacheabilityUsesInjectedBudget();
    void animationHandlingControlsPlannedEffects();
    void staticDecodedImagesAreAppliedToPresentation();
    void staticDecodedImagesPublishProviderSourceAndKeepCompatibilitySurface();
    void unpresentableDecodedImagesLeaveExistingPresentationUntouched();
    void streamedAnimationImagesPresentFirstFrames();
    void animationFirstFramesPublishProviderSourceAndKeepCompatibilitySurface();
    void animationPlaybackFramesPublishProviderSourceAndKeepCompatibilitySurface();
    void secondaryAnimationModePresentsFirstFrame();
};

void TestImagePresentationLoad::predecodedImagesPlanStaticCacheablePresentation()
{
    KiriView::PredecodedImage image {
        staticDisplayTestImagePayload(testImage(QSize(9, 5)), testImage(QSize(3, 2)),
            KiriView::StaticImageDisplayHints { 0.5 }, KiriView::DisplayImageQuality::FirstDisplay),
        KiriView::DisplayedImageLocation::fromUrl(localUrl(QStringLiteral("/images/page.png"))),
    };

    const KiriView::ImagePresentationLoadPlan plan
        = KiriView::planPredecodedImagePresentationLoad(std::move(image));

    QVERIFY(plan.hasPresentation());
    const auto *load = planPayload<KiriView::ImagePresentationStaticImageLoad>(plan);
    QVERIFY(load != nullptr);
    QVERIFY(load->predecodeCacheable);
    QVERIFY(load->displayImage.has_value());
    QCOMPARE(load->displayImage->sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(load->displayImage->originalSize, QSize(9, 5));
    QCOMPARE(load->displayImage->image.size(), QSize(3, 2));
    QCOMPARE(load->displayImage->quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(load->displayImage->displayPixelsPerSourcePixel, 0.5);
    QVERIFY(load->staticImage.isValid());
    QCOMPARE(load->staticImage.source->imageSize(), QSize(9, 5));
    QCOMPARE(load->staticImage.preview.size(), QSize(3, 2));
}

void TestImagePresentationLoad::predecodedImagesPublishProviderSourceAndKeepCompatibilitySurface()
{
    auto displayImageStore = std::make_shared<KiriView::DisplayImageStore>(1024 * 1024);
    KiriView::ImagePageSurfaceController controller
        = pageSurfaceController(this, displayImageStore);
    KiriView::PredecodedImage image {
        staticDisplayTestImagePayload(testImage(QSize(12, 8)), testImage(QSize(6, 4)),
            KiriView::StaticImageDisplayHints { 0.5 }, KiriView::DisplayImageQuality::FirstDisplay),
        KiriView::DisplayedImageLocation::fromUrl(localUrl(QStringLiteral("/images/page.png"))),
    };

    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentPredecodedImageLoad(controller, std::move(image), renderContext());

    QVERIFY(result.presented);
    QVERIFY(controller.hasImage());
    QVERIFY(controller.imageSurface() != nullptr);
    QVERIFY(controller.imageSurface()->legacyFrameSurface() != nullptr
        || controller.imageSurface()->staticTileSurface() != nullptr);

    const KiriView::ImageDisplaySourceSlot displaySource = controller.snapshot().displaySource;
    QCOMPARE(displaySource.status, KiriView::ImageDisplaySourceStatus::Ready);
    QVERIFY(!displaySource.providerUrl.isEmpty());
    QCOMPARE(displaySource.sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(displaySource.originalSize, QSize(12, 8));
    QCOMPARE(displaySource.rasterSize, QSize(6, 4));
    QCOMPARE(displaySource.sourceSizeHint, QSize(6, 0));
    QCOMPARE(displaySource.quality, KiriView::DisplayImageQuality::FirstDisplay);

    const QString entryId = displaySource.providerUrl.path().mid(1);
    const std::optional<KiriView::DisplayImageStoreEntry> stored
        = displayImageStore->entry(entryId);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->originalSize, QSize(12, 8));
    QCOMPARE(stored->rasterSize, QSize(6, 4));
    QCOMPARE(stored->quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(stored->sourceIdentity, QStringLiteral("test-image"));
}

void TestImagePresentationLoad::decodedImagesPlanPresentationActions()
{
    {
        KiriView::DecodedImage decoded = staticDecodedTestImage(testImage(QSize(12, 8)));
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation,
            testPredecodeCacheByteBudget);

        QVERIFY(plan.hasPresentation());
        const auto *load = planPayload<KiriView::ImagePresentationStaticImageLoad>(plan);
        QVERIFY(load != nullptr);
        QVERIFY(load->predecodeCacheable);
        QCOMPARE(load->staticImage.source->imageSize(), QSize(12, 8));
    }

    {
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {};
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation,
            testPredecodeCacheByteBudget);

        QVERIFY(!plan.hasPresentation());
        QVERIFY(std::holds_alternative<std::monostate>(plan.payload));
    }
}

void TestImagePresentationLoad::staticDecodedPredecodeCacheabilityUsesInjectedBudget()
{
    KiriView::DecodedImage decoded = staticDecodedTestImage(testImage(QSize(12, 8)));
    const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
        std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation, 1);

    const auto *load = planPayload<KiriView::ImagePresentationStaticImageLoad>(plan);
    QVERIFY(load != nullptr);
    QVERIFY(!load->predecodeCacheable);
}

void TestImagePresentationLoad::animationHandlingControlsPlannedEffects()
{
    {
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
            testImage(QSize(13, 7)),
            QByteArrayLiteral("apng-data"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::FirstFrameOnly,
            testPredecodeCacheByteBudget);

        const auto *load = planPayload<KiriView::ImagePresentationFrameLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->frame.size(), QSize(13, 7));
    }

    {
        KiriView::DecodedImage decoded = KiriView::ReaderAnimationImage {
            testImage(QSize(10, 6)),
            QByteArrayLiteral("reader-data"),
            QByteArrayLiteral("gif"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation,
            testPredecodeCacheByteBudget);

        const auto *load = planPayload<KiriView::ImagePresentationAnimationLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->firstFrame.size(), QSize(10, 6));
        const auto *request
            = std::get_if<KiriView::ReaderAnimationPlaybackRequest>(&load->playback.payload);
        QVERIFY(request != nullptr);
        QCOMPARE(request->data, QByteArrayLiteral("reader-data"));
        QCOMPARE(request->format, QByteArrayLiteral("gif"));
    }

    {
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
            testImage(QSize(14, 8)),
            QByteArrayLiteral("apng-data"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation,
            testPredecodeCacheByteBudget);

        const auto *load = planPayload<KiriView::ImagePresentationAnimationLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->firstFrame.size(), QSize(14, 8));
        const auto *request
            = std::get_if<KiriView::ApngAnimationPlaybackRequest>(&load->playback.payload);
        QVERIFY(request != nullptr);
        QCOMPARE(request->data, QByteArrayLiteral("apng-data"));
    }

    {
        KiriView::DecodedImage decoded = KiriView::HeifSequenceAnimationImage {
            testImage(QSize(11, 5)),
            QByteArrayLiteral("heif-data"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation,
            testPredecodeCacheByteBudget);

        const auto *load = planPayload<KiriView::ImagePresentationAnimationLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->firstFrame.size(), QSize(11, 5));
        const auto *request
            = std::get_if<KiriView::HeifSequenceAnimationPlaybackRequest>(&load->playback.payload);
        QVERIFY(request != nullptr);
        QCOMPARE(request->data, QByteArrayLiteral("heif-data"));
    }
}

void TestImagePresentationLoad::staticDecodedImagesAreAppliedToPresentation()
{
    KiriView::ImagePageSurfaceController controller = pageSurfaceController(this);
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    KiriView::DecodedImage decoded = staticDecodedTestImage(testImage(QSize(12, 8)));
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::StartAnimation, renderContext());

    QVERIFY(result.presented);
    QCOMPARE(result.imageSize, QSize(12, 8));
    QCOMPARE(controller.imageSize(), QSize(12, 8));
    QVERIFY(controller.hasImage());
    QVERIFY(controller.isPredecodeCacheable());
}

void TestImagePresentationLoad::
    staticDecodedImagesPublishProviderSourceAndKeepCompatibilitySurface()
{
    auto displayImageStore = std::make_shared<KiriView::DisplayImageStore>(1024 * 1024);
    KiriView::ImagePageSurfaceController controller
        = pageSurfaceController(this, displayImageStore);
    KiriView::DecodedImage decoded = KiriView::StaticDecodedImage {
        staticDisplayTestImagePayload(KiriView::TestSupport::testImage(QSize(12, 8)),
            KiriView::TestSupport::testImage(QSize(6, 4)), {},
            KiriView::DisplayImageQuality::FirstDisplay),
    };

    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::StartAnimation, renderContext());

    QVERIFY(result.presented);
    QVERIFY(controller.hasImage());
    QVERIFY(controller.imageSurface() != nullptr);
    QVERIFY(controller.imageSurface()->legacyFrameSurface() != nullptr
        || controller.imageSurface()->staticTileSurface() != nullptr);

    const KiriView::ImageDisplaySourceSlot displaySource = controller.snapshot().displaySource;
    QCOMPARE(displaySource.status, KiriView::ImageDisplaySourceStatus::Ready);
    QVERIFY(!displaySource.providerUrl.isEmpty());
    QCOMPARE(displaySource.revision, quint64(1));
    QCOMPARE(displaySource.sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(displaySource.originalSize, QSize(12, 8));
    QCOMPARE(displaySource.rasterSize, QSize(6, 4));
    QCOMPARE(displaySource.sourceSizeHint, QSize(6, 0));
    QCOMPARE(displaySource.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QVERIFY(!displaySource.cacheEnabled);
    QVERIFY(displaySource.loadAcknowledgmentRequired);

    const QString entryId = displaySource.providerUrl.path().mid(1);
    const std::optional<KiriView::DisplayImageStoreEntry> stored
        = displayImageStore->entry(entryId);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->originalSize, QSize(12, 8));
    QCOMPARE(stored->rasterSize, QSize(6, 4));
    QCOMPARE(stored->quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(stored->sourceIdentity, QStringLiteral("test-image"));
}

void TestImagePresentationLoad::unpresentableDecodedImagesLeaveExistingPresentationUntouched()
{
    KiriView::ImagePageSurfaceController controller = pageSurfaceController(this);
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);
    KiriView::DecodedImage decoded = staticDecodedTestImage(testImage(QSize(12, 8)));
    QVERIFY(KiriView::presentDecodedImageLoad(controller, std::move(decoded),
        KiriView::ImagePresentationAnimationHandling::StartAnimation, renderContext())
            .presented);

    KiriView::DecodedImage unpresentable = KiriView::ApngAnimationImage {};
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(unpresentable),
            KiriView::ImagePresentationAnimationHandling::StartAnimation, renderContext());

    QVERIFY(!result.presented);
    QCOMPARE(controller.imageSize(), QSize(12, 8));
    QVERIFY(controller.hasImage());
}

void TestImagePresentationLoad::streamedAnimationImagesPresentFirstFrames()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/animated.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    {
        KiriView::ImagePageSurfaceController controller = pageSurfaceController(this);
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
            testImage(QSize(13, 7)),
            QByteArrayLiteral("apng-data"),
        };

        const KiriView::ImagePresentationLoadResult result
            = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
                KiriView::ImagePresentationAnimationHandling::FirstFrameOnly, renderContext());

        QVERIFY(result.presented);
        QCOMPARE(result.imageSize, QSize(13, 7));
        QCOMPARE(controller.imageSize(), QSize(13, 7));
        QVERIFY(!controller.isPredecodeCacheable());
    }

    {
        KiriView::ImagePageSurfaceController controller = pageSurfaceController(this);
        KiriView::DecodedImage decoded = KiriView::HeifSequenceAnimationImage {
            testImage(QSize(11, 5)),
            QByteArrayLiteral("heif-data"),
        };

        const KiriView::ImagePresentationLoadResult result
            = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
                KiriView::ImagePresentationAnimationHandling::FirstFrameOnly, renderContext());

        QVERIFY(result.presented);
        QCOMPARE(result.imageSize, QSize(11, 5));
        QCOMPARE(controller.imageSize(), QSize(11, 5));
        QVERIFY(!controller.isPredecodeCacheable());
    }
}

void TestImagePresentationLoad::
    animationFirstFramesPublishProviderSourceAndKeepCompatibilitySurface()
{
    auto displayImageStore = std::make_shared<KiriView::DisplayImageStore>(1024 * 1024);
    KiriView::ImagePageSurfaceController controller
        = pageSurfaceController(this, displayImageStore);
    const QString sourceIdentity = QStringLiteral("file:///tmp/animated.gif");
    KiriView::DecodedImage decoded = KiriView::ReaderAnimationImage {
        testImage(QSize(10, 6)),
        QByteArrayLiteral("reader-data"),
        QByteArrayLiteral("gif"),
        {},
    };
    assignDecodedSourceIdentity(decoded, sourceIdentity);

    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::FirstFrameOnly, renderContext());

    QVERIFY(result.presented);
    QCOMPARE(result.imageSize, QSize(10, 6));
    QVERIFY(controller.hasImage());
    QVERIFY(controller.imageSurface() != nullptr);
    const KiriView::LegacyFrameSurface *legacySurface
        = controller.imageSurface()->legacyFrameSurface();
    QVERIFY(legacySurface != nullptr);
    QCOMPARE(legacySurface->image.size(), QSize(10, 6));

    const KiriView::ImageDisplaySourceSlot displaySource = controller.snapshot().displaySource;
    QCOMPARE(displaySource.status, KiriView::ImageDisplaySourceStatus::Ready);
    QVERIFY(!displaySource.providerUrl.isEmpty());
    QCOMPARE(displaySource.revision, quint64(1));
    QCOMPARE(displaySource.sourceIdentity, sourceIdentity);
    QCOMPARE(displaySource.originalSize, QSize(10, 6));
    QCOMPARE(displaySource.rasterSize, QSize(10, 6));
    QCOMPARE(displaySource.sourceSizeHint, QSize());
    QCOMPARE(displaySource.quality, KiriView::DisplayImageQuality::Exact);
    QVERIFY(!displaySource.cacheEnabled);
    QVERIFY(displaySource.loadAcknowledgmentRequired);

    const std::optional<KiriView::DisplayImageStoreEntry> stored
        = displayImageStore->entry(entryId(displaySource));
    QVERIFY(stored.has_value());
    QCOMPARE(stored->sourceIdentity, sourceIdentity);
    QCOMPARE(stored->originalSize, QSize(10, 6));
    QCOMPARE(stored->rasterSize, QSize(10, 6));
    QCOMPARE(stored->quality, KiriView::DisplayImageQuality::Exact);
    QCOMPARE(stored->debugLabel, QStringLiteral("animation-frame"));
    QCOMPARE(stored->previewOrigin, KiriView::DisplayImagePreviewOrigin::None);
}

void TestImagePresentationLoad::
    animationPlaybackFramesPublishProviderSourceAndKeepCompatibilitySurface()
{
    auto displayImageStore = std::make_shared<KiriView::DisplayImageStore>(1024 * 1024);
    KiriView::ImagePageSurfaceController controller
        = pageSurfaceController(this, displayImageStore);
    const QString sourceIdentity = QStringLiteral("file:///tmp/animated.apng");
    const QColor firstColor(Qt::red);
    const QColor secondColor(Qt::blue);
    const QByteArray apng = makeApng({ fullCanvasFrame(firstColor), fullCanvasFrame(secondColor) });
    KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
        solidRgbaImage(firstColor),
        apng,
        {},
    };
    assignDecodedSourceIdentity(decoded, sourceIdentity);

    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::StartAnimation, renderContext());

    QVERIFY(result.presented);
    const KiriView::ImageDisplaySourceSlot firstDisplay = controller.snapshot().displaySource;
    QCOMPARE(firstDisplay.status, KiriView::ImageDisplaySourceStatus::Ready);
    QVERIFY(!firstDisplay.providerUrl.isEmpty());
    QCOMPARE(firstDisplay.sourceIdentity, sourceIdentity);
    QCOMPARE(firstDisplay.revision, quint64(1));
    QVERIFY(displayImageStore->entry(entryId(firstDisplay)).has_value());

    QTRY_VERIFY_WITH_TIMEOUT(
        controller.snapshot().displaySource.providerUrl != firstDisplay.providerUrl, 300);
    const KiriView::ImageDisplaySourceSlot nextDisplay = controller.snapshot().displaySource;
    QCOMPARE(nextDisplay.status, KiriView::ImageDisplaySourceStatus::Ready);
    QCOMPARE(nextDisplay.revision, firstDisplay.revision + 1);
    QCOMPARE(nextDisplay.sourceIdentity, sourceIdentity);
    QCOMPARE(nextDisplay.originalSize, QSize(1, 1));
    QCOMPARE(nextDisplay.rasterSize, QSize(1, 1));
    QCOMPARE(nextDisplay.quality, KiriView::DisplayImageQuality::Exact);
    QVERIFY(nextDisplay.loadAcknowledgmentRequired);
    QVERIFY(!displayImageStore->entry(entryId(firstDisplay)).has_value());

    const std::optional<KiriView::DisplayImageStoreEntry> stored
        = displayImageStore->entry(entryId(nextDisplay));
    QVERIFY(stored.has_value());
    QCOMPARE(stored->debugLabel, QStringLiteral("animation-frame"));
    QCOMPARE(stored->image.pixelColor(0, 0), secondColor);

    QVERIFY(controller.imageSurface() != nullptr);
    const KiriView::LegacyFrameSurface *legacySurface
        = controller.imageSurface()->legacyFrameSurface();
    QVERIFY(legacySurface != nullptr);
    QCOMPARE(legacySurface->image.pixelColor(0, 0), secondColor);
}

void TestImagePresentationLoad::secondaryAnimationModePresentsFirstFrame()
{
    KiriView::ImagePageSurfaceController controller = pageSurfaceController(this);
    const QUrl imageUrl = localUrl(QStringLiteral("/images/animated.gif"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    KiriView::DecodedImage decoded = KiriView::ReaderAnimationImage {
        testImage(QSize(10, 6)),
        QByteArrayLiteral("reader-data"),
        QByteArrayLiteral("gif"),
    };
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::FirstFrameOnly, renderContext());

    QVERIFY(result.presented);
    QCOMPARE(result.imageSize, QSize(10, 6));
    QCOMPARE(controller.imageSize(), QSize(10, 6));
    QVERIFY(controller.hasImage());
}

QTEST_GUILESS_MAIN(TestImagePresentationLoad)

#include "test_imagepresentationload.moc"
