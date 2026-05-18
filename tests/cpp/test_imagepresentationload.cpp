// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationload.h"

#include "image_test_support.h"
#include "imagepresentationcontroller.h"
#include "imagerendering.h"

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <utility>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::ImageLoadSession loadSession(const QUrl &url)
{
    return KiriView::ImageLoadSession {
        1,
        KiriView::ImageLoadRequest::fromUrl(url),
        KiriView::DisplayedImageLocation::fromUrl(url),
    };
}

KiriView::ImagePresentationController presentationController(QObject *parent)
{
    return KiriView::ImagePresentationController(parent,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {});
}
}

class TestImagePresentationLoad : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticDecodedImagesAreAppliedToPresentation();
    void unpresentableDecodedImagesLeaveExistingPresentationUntouched();
    void secondaryAnimationModePresentsFirstFrame();
};

void TestImagePresentationLoad::staticDecodedImagesAreAppliedToPresentation()
{
    KiriView::ImagePresentationController controller = presentationController(this);
    controller.setViewportSize(QSizeF(200.0, 100.0));
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    KiriView::DecodedImage decoded
        = KiriView::StaticDecodedImage { staticTestImagePayload(testImage(QSize(12, 8))) };
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, session, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::StartAnimation);

    QVERIFY(result.presented);
    QCOMPARE(result.imageSize, QSize(12, 8));
    QCOMPARE(controller.imageSize(), QSize(12, 8));
    QVERIFY(controller.hasImage());
}

void TestImagePresentationLoad::unpresentableDecodedImagesLeaveExistingPresentationUntouched()
{
    KiriView::ImagePresentationController controller = presentationController(this);
    controller.setViewportSize(QSizeF(200.0, 100.0));
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);
    KiriView::DecodedImage decoded
        = KiriView::StaticDecodedImage { staticTestImagePayload(testImage(QSize(12, 8))) };
    QVERIFY(KiriView::presentDecodedImageLoad(controller, session, std::move(decoded),
        KiriView::ImagePresentationAnimationHandling::StartAnimation)
            .presented);

    KiriView::DecodedImage unpresentable = KiriView::ApngAnimationImage {};
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, session, std::move(unpresentable),
            KiriView::ImagePresentationAnimationHandling::StartAnimation);

    QVERIFY(!result.presented);
    QCOMPARE(controller.imageSize(), QSize(12, 8));
    QVERIFY(controller.hasImage());
}

void TestImagePresentationLoad::secondaryAnimationModePresentsFirstFrame()
{
    KiriView::ImagePresentationController controller = presentationController(this);
    controller.setViewportSize(QSizeF(200.0, 100.0));
    const QUrl imageUrl = localUrl(QStringLiteral("/images/animated.gif"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    KiriView::DecodedImage decoded = KiriView::ReaderAnimationImage {
        testImage(QSize(10, 6)),
        QByteArrayLiteral("reader-data"),
        QByteArrayLiteral("gif"),
    };
    const KiriView::ImagePresentationLoadResult result
        = KiriView::presentDecodedImageLoad(controller, session, std::move(decoded),
            KiriView::ImagePresentationAnimationHandling::FirstFrameOnly);

    QVERIFY(result.presented);
    QCOMPARE(result.imageSize, QSize(10, 6));
    QCOMPARE(controller.imageSize(), QSize(10, 6));
    QVERIFY(controller.hasImage());
}

QTEST_GUILESS_MAIN(TestImagePresentationLoad)

#include "test_imagepresentationload.moc"
