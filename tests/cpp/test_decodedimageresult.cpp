// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/decodedimageresult.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <utility>
#include <variant>

class TestDecodedImageResult : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void exposesFailurePayload();
    void exposesImagePayload();
    void takeImageMovesImagePayloadOnly();
    void staticMetadataMirrorsIntoDisplayPayload();
};

void TestDecodedImageResult::exposesFailurePayload()
{
    const kiriview::DecodedImageResult result
        = kiriview::failedDecodedImageResult(QStringLiteral("decode failed"));

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("decode failed"));
    QVERIFY(kiriview::decodedImageResultImage(result) == nullptr);
}

void TestDecodedImageResult::exposesImagePayload()
{
    const kiriview::DecodedImageResult result
        = kiriview::successfulDecodedImageResult(kiriview::TestSupport::staticDecodedTestImage());

    QVERIFY(kiriview::decodedImageResultFailure(result) == nullptr);
    const kiriview::DecodedImage *image = kiriview::decodedImageResultImage(result);
    QVERIFY(image != nullptr);
    QVERIFY(std::get_if<kiriview::StaticDecodedImage>(image) != nullptr);
}

void TestDecodedImageResult::takeImageMovesImagePayloadOnly()
{
    std::optional<kiriview::DecodedImage> image
        = kiriview::successfulDecodedImageResult(kiriview::TestSupport::staticDecodedTestImage())
              .takeImage();
    QVERIFY(image.has_value());
    QVERIFY(std::get_if<kiriview::StaticDecodedImage>(&*image) != nullptr);

    image = kiriview::failedDecodedImageResult(QStringLiteral("decode failed")).takeImage();
    QVERIFY(!image.has_value());
}

void TestDecodedImageResult::staticMetadataMirrorsIntoDisplayPayload()
{
    kiriview::DecodedImage image = kiriview::TestSupport::staticDecodedTestImage();
    kiriview::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera");

    kiriview::setDecodedImageEmbeddedMetadata(image, metadata);

    const kiriview::StaticDecodedImage *decoded = std::get_if<kiriview::StaticDecodedImage>(&image);
    QVERIFY(decoded != nullptr);
    QCOMPARE(
        kiriview::decodedImageEmbeddedMetadata(image).cameraMake, QStringLiteral("Kiri Camera"));
    QCOMPARE(decoded->displayImage.embeddedMetadata.cameraMake, QStringLiteral("Kiri Camera"));
}

QTEST_GUILESS_MAIN(TestDecodedImageResult)

#include "test_decodedimageresult.moc"
