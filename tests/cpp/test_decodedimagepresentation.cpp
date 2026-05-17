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
                2,
                30,
            },
        });
    const auto *readerPresentation
        = std::get_if<KiriView::DecodedAnimationImagePresentation>(&presentation);
    QVERIFY(readerPresentation != nullptr);
    QCOMPARE(readerPresentation->kind, KiriView::DecodedImageAnimationKind::Reader);
    QCOMPARE(readerPresentation->firstFrame.size(), KiriView::TestSupport::testImage().size());
    QCOMPARE(readerPresentation->data, QByteArrayLiteral("reader-data"));
    QCOMPARE(readerPresentation->format, QByteArrayLiteral("gif"));
    QCOMPARE(readerPresentation->loopCount, 2);
    QCOMPARE(readerPresentation->firstFrameDelay, 30);

    presentation = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage {
        KiriView::ApngAnimationImage {
            KiriView::TestSupport::testImage(),
            QByteArrayLiteral("apng-data"),
            40,
            3,
        },
    });
    const auto *apngPresentation
        = std::get_if<KiriView::DecodedAnimationImagePresentation>(&presentation);
    QVERIFY(apngPresentation != nullptr);
    QCOMPARE(apngPresentation->kind, KiriView::DecodedImageAnimationKind::Apng);
    QCOMPARE(apngPresentation->data, QByteArrayLiteral("apng-data"));
    QCOMPARE(apngPresentation->loopCount, 3);
    QCOMPARE(apngPresentation->firstFrameDelay, 40);

    presentation = KiriView::decodedImagePresentationForImage(KiriView::DecodedImage {
        KiriView::HeifSequenceAnimationImage {
            KiriView::TestSupport::testImage(),
            QByteArrayLiteral("heif-data"),
            50,
        },
    });
    const auto *heifPresentation
        = std::get_if<KiriView::DecodedAnimationImagePresentation>(&presentation);
    QVERIFY(heifPresentation != nullptr);
    QCOMPARE(heifPresentation->kind, KiriView::DecodedImageAnimationKind::HeifSequence);
    QCOMPARE(heifPresentation->data, QByteArrayLiteral("heif-data"));
    QCOMPARE(heifPresentation->firstFrameDelay, 50);

    presentation = KiriView::decodedImagePresentationForImage(
        KiriView::DecodedImage { KiriView::ApngAnimationImage {} });
    QVERIFY(std::holds_alternative<KiriView::UnpresentableDecodedImage>(presentation));
}

QTEST_GUILESS_MAIN(TestDecodedImagePresentation)

#include "test_decodedimagepresentation.moc"
