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
    const KiriView::DecodedImageResult result
        = KiriView::failedDecodedImageResult(QStringLiteral("decode failed"));

    const KiriView::DecodedImageFailure *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("decode failed"));
    QVERIFY(KiriView::decodedImageResultImage(result) == nullptr);
}

void TestDecodedImageResult::exposesImagePayload()
{
    const KiriView::DecodedImageResult result
        = KiriView::successfulDecodedImageResult(KiriView::TestSupport::staticDecodedTestImage());

    QVERIFY(KiriView::decodedImageResultFailure(result) == nullptr);
    const KiriView::DecodedImage *image = KiriView::decodedImageResultImage(result);
    QVERIFY(image != nullptr);
    QVERIFY(std::get_if<KiriView::StaticDecodedImage>(image) != nullptr);
}

void TestDecodedImageResult::takeImageMovesImagePayloadOnly()
{
    std::optional<KiriView::DecodedImage> image
        = KiriView::successfulDecodedImageResult(KiriView::TestSupport::staticDecodedTestImage())
              .takeImage();
    QVERIFY(image.has_value());
    QVERIFY(std::get_if<KiriView::StaticDecodedImage>(&*image) != nullptr);

    image = KiriView::failedDecodedImageResult(QStringLiteral("decode failed")).takeImage();
    QVERIFY(!image.has_value());
}

void TestDecodedImageResult::staticMetadataMirrorsIntoDisplayPayload()
{
    KiriView::DecodedImage image = KiriView::TestSupport::staticDecodedTestImage();
    KiriView::EmbeddedMetadata metadata;
    metadata.cameraMake = QStringLiteral("Kiri Camera");

    KiriView::setDecodedImageEmbeddedMetadata(image, metadata);

    const KiriView::StaticDecodedImage *decoded = std::get_if<KiriView::StaticDecodedImage>(&image);
    QVERIFY(decoded != nullptr);
    QCOMPARE(
        KiriView::decodedImageEmbeddedMetadata(image).cameraMake, QStringLiteral("Kiri Camera"));
    QCOMPARE(decoded->displayImage.embeddedMetadata.cameraMake, QStringLiteral("Kiri Camera"));
}

QTEST_GUILESS_MAIN(TestDecodedImageResult)

#include "test_decodedimageresult.moc"
