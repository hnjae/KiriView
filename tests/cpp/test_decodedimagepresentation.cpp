// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "image_test_support.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <Qt>
#include <utility>
#include <variant>

class TestDecodedImagePresentation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticImagesCarryCacheability();
    void streamedAnimationsUseFirstFrames();
};

void TestDecodedImagePresentation::staticImagesCarryCacheability()
{
    KiriView::StaticDecodedImage decoded {
        KiriView::TestSupport::staticTestImagePayload(KiriView::TestSupport::testImage()),
    };

    KiriView::DecodedImagePresentation presentation
        = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage { std::move(decoded) });
    const auto *staticPresentation
        = std::get_if<KiriView::DecodedStaticImagePresentation>(&presentation);
    QVERIFY(staticPresentation != nullptr);
    QVERIFY(staticPresentation->predecodeCacheable);

    presentation = KiriView::decodedImagePresentationForImage(
        KiriView::DecodedImage { KiriView::StaticDecodedImage {} });
    staticPresentation = std::get_if<KiriView::DecodedStaticImagePresentation>(&presentation);
    QVERIFY(staticPresentation != nullptr);
    QVERIFY(!staticPresentation->predecodeCacheable);
}

void TestDecodedImagePresentation::streamedAnimationsUseFirstFrames()
{
    KiriView::DecodedImagePresentation presentation
        = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage {
            KiriView::ReaderAnimationImage {
                KiriView::TestSupport::testImage(),
                QByteArrayLiteral("reader-data"),
                QByteArrayLiteral("gif"),
            },
        });
    const auto *readerPresentation
        = std::get_if<KiriView::DecodedReaderAnimationPresentation>(&presentation);
    QVERIFY(readerPresentation != nullptr);
    QCOMPARE(readerPresentation->firstFrame.size(), KiriView::TestSupport::testImage().size());
    QCOMPARE(readerPresentation->data, QByteArrayLiteral("reader-data"));
    QCOMPARE(readerPresentation->format, QByteArrayLiteral("gif"));

    presentation = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage {
        KiriView::ApngAnimationImage {
            KiriView::TestSupport::testImage(),
            QByteArrayLiteral("apng-data"),
        },
    });
    const auto *apngPresentation
        = std::get_if<KiriView::DecodedApngAnimationPresentation>(&presentation);
    QVERIFY(apngPresentation != nullptr);
    QCOMPARE(apngPresentation->data, QByteArrayLiteral("apng-data"));

    presentation = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage {
        KiriView::HeifSequenceAnimationImage {
            KiriView::TestSupport::testImage(),
            QByteArrayLiteral("heif-data"),
        },
    });
    const auto *heifPresentation
        = std::get_if<KiriView::DecodedHeifSequenceAnimationPresentation>(&presentation);
    QVERIFY(heifPresentation != nullptr);
    QCOMPARE(heifPresentation->data, QByteArrayLiteral("heif-data"));

    presentation = KiriView::decodedImagePresentationForImage(
        KiriView::DecodedImage { KiriView::ApngAnimationImage {} });
    QVERIFY(std::holds_alternative<KiriView::UnpresentableDecodedImage>(presentation));
}

QTEST_GUILESS_MAIN(TestDecodedImagePresentation)

#include "test_decodedimagepresentation.moc"
