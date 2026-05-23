// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_IMAGE_TEST_SUPPORT_H

#include "candidate_test_support.h"
#include "document/imagedocumentruntimedependencies.h"
#include "image_async_test_support.h"
#include "rendering/staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <QtGlobal>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace KiriView::TestSupport {
namespace Detail {
    inline void appendFourCc(QByteArray &data, std::string_view fourCc)
    {
        Q_ASSERT(fourCc.size() == 4);

        if (fourCc.size() >= 4) {
            data.append(fourCc.data(), 4);
            return;
        }

        data.append(fourCc.data(), static_cast<qsizetype>(fourCc.size()));
        data.append(4 - static_cast<qsizetype>(fourCc.size()), '\0');
    }
}

inline QByteArray heifFtypBox(
    std::string_view majorBrand, std::initializer_list<std::string_view> compatibleBrands)
{
    const quint32 boxSize = 16 + static_cast<quint32>(compatibleBrands.size() * 4);
    QByteArray data;
    data.append(static_cast<char>((boxSize >> 24) & 0xff));
    data.append(static_cast<char>((boxSize >> 16) & 0xff));
    data.append(static_cast<char>((boxSize >> 8) & 0xff));
    data.append(static_cast<char>(boxSize & 0xff));
    Detail::appendFourCc(data, "ftyp");
    Detail::appendFourCc(data, majorBrand);
    data.append(4, '\0');
    for (std::string_view brand : compatibleBrands) {
        Detail::appendFourCc(data, brand);
    }
    return data;
}

inline QImage testImage(const QSize &size = QSize(1, 1))
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

inline QImage testImage(int width, int height = 1) { return testImage(QSize(width, height)); }

class TestImageTileSource final : public ImageTileSource
{
public:
    explicit TestImageTileSource(QImage image)
        : m_image(std::move(image))
    {
    }

    QSize imageSize() const override { return m_image.size(); }
    qsizetype byteCost() const override { return m_image.sizeInBytes(); }

    QImage decodeBlockingDisplayImage(int, QString *) const override { return m_image; }

    std::optional<DecodedTile> decodeTile(const TileRequest &request, QString *) const override
    {
        if (request.textureLevelRect.isEmpty()) {
            return std::nullopt;
        }

        QImage levelImage
            = m_image.scaled(request.levelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        return DecodedTile { request.key, request.levelSize, request.levelRect,
            request.textureLevelRect, levelImage.copy(request.textureLevelRect),
            request.displaySourceRect, request.displaySourceRectF };
    }

private:
    QImage m_image;
};

inline StaticImagePayload staticTestImagePayload(
    const QImage &sourceImage, const QImage &preview, StaticImageDisplayHints displayHints = {})
{
    return StaticImagePayload {
        std::make_shared<TestImageTileSource>(sourceImage),
        preview,
        displayHints,
    };
}

inline StaticImagePayload staticTestImagePayload(
    const QImage &image = testImage(), StaticImageDisplayHints displayHints = {})
{
    return staticTestImagePayload(image, image, displayHints);
}

inline StaticDecodedImage staticDecodedTestImage(const QImage &image = testImage())
{
    return StaticDecodedImage { staticTestImagePayload(image) };
}

inline QString testImageDecodeFailureString() { return QStringLiteral("decode failed"); }

inline DecodedImageResult failedTestImageDecodeResult()
{
    return failedDecodedImageResult(testImageDecodeFailureString());
}

inline ImageDataDecoder staticImageDataDecoder(QImage image = testImage())
{
    return [image = std::move(image)](const QByteArray &, const ImageDecodeRequest &) {
        return successfulDecodedImageResult(staticDecodedTestImage(image));
    };
}

inline ImageDataDecoder staticImageDataDecoderRejectingBadData(QImage image = testImage())
{
    return [decoder = staticImageDataDecoder(std::move(image))](
               const QByteArray &data, const ImageDecodeRequest &request) {
        if (data == QByteArrayLiteral("bad")) {
            return failedTestImageDecodeResult();
        }

        return decoder(data, request);
    };
}

inline ImageDecodeDependencies imageDecodeDependenciesFor(
    ManualImageDataLoader &dataLoader, ImageDataDecoder dataDecoder)
{
    return ImageDecodeDependencies {
        dataLoaderFor(dataLoader),
        std::move(dataDecoder),
    };
}

inline ImageDocumentRuntimeDependencyOverrides imageDocumentRuntimeDependencyOverridesFor(
    FakeImageNavigationCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    ImageDataDecoder dataDecoder, FileOperationProvider fileOperations = {})
{
    return ImageDocumentRuntimeDependencyOverrides {
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, std::move(dataDecoder)),
        std::move(fileOperations),
    };
}

}

#endif
