// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "image_test_support.h"
#include "presentation/imagepresentationcontroller.h"
#include "rendering/imagerendering.h"

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <utility>
#include <variant>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::ImageLoadSession loadSession(const QUrl &url)
{
    return KiriView::ImageLoadSession(1, KiriView::ImageLoadRequest::fromUrl(url),
        KiriView::DisplayedImageLocation::fromUrl(url));
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
    void decodedImagesPlanPresentationActions();
    void animationHandlingControlsPlannedEffects();
    void staticDecodedImagesAreAppliedToPresentation();
    void unpresentableDecodedImagesLeaveExistingPresentationUntouched();
    void streamedAnimationImagesPresentFirstFrames();
    void secondaryAnimationModePresentsFirstFrame();
};

void TestImagePresentationLoad::predecodedImagesPlanStaticCacheablePresentation()
{
    KiriView::PredecodedImage image {
        staticTestImagePayload(testImage(QSize(9, 5))),
        KiriView::DisplayedImageLocation::fromUrl(localUrl(QStringLiteral("/images/page.png"))),
    };

    const KiriView::ImagePresentationLoadPlan plan
        = KiriView::planPredecodedImagePresentationLoad(std::move(image));

    QVERIFY(plan.hasPresentation());
    const auto *load = planPayload<KiriView::ImagePresentationStaticImageLoad>(plan);
    QVERIFY(load != nullptr);
    QVERIFY(load->predecodeCacheable);
    QVERIFY(load->staticImage.isValid());
    QCOMPARE(load->staticImage.source->imageSize(), QSize(9, 5));
}

void TestImagePresentationLoad::decodedImagesPlanPresentationActions()
{
    {
        KiriView::DecodedImage decoded
            = KiriView::StaticDecodedImage { staticTestImagePayload(testImage(QSize(12, 8))) };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation);

        QVERIFY(plan.hasPresentation());
        const auto *load = planPayload<KiriView::ImagePresentationStaticImageLoad>(plan);
        QVERIFY(load != nullptr);
        QVERIFY(load->predecodeCacheable);
        QCOMPARE(load->staticImage.source->imageSize(), QSize(12, 8));
    }

    {
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {};
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation);

        QVERIFY(!plan.hasPresentation());
        QVERIFY(std::holds_alternative<std::monostate>(plan.payload));
    }
}

void TestImagePresentationLoad::animationHandlingControlsPlannedEffects()
{
    {
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
            testImage(QSize(13, 7)),
            QByteArrayLiteral("apng-data"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::FirstFrameOnly);

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
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation);

        const auto *load = planPayload<KiriView::ImagePresentationReaderAnimationLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->firstFrame.size(), QSize(10, 6));
        QCOMPARE(load->data, QByteArrayLiteral("reader-data"));
        QCOMPARE(load->format, QByteArrayLiteral("gif"));
    }

    {
        KiriView::DecodedImage decoded = KiriView::HeifSequenceAnimationImage {
            testImage(QSize(11, 5)),
            QByteArrayLiteral("heif-data"),
        };
        const KiriView::ImagePresentationLoadPlan plan = KiriView::planDecodedImagePresentationLoad(
            std::move(decoded), KiriView::ImagePresentationAnimationHandling::StartAnimation);

        const auto *load = planPayload<KiriView::ImagePresentationHeifSequenceAnimationLoad>(plan);
        QVERIFY(load != nullptr);
        QCOMPARE(load->firstFrame.size(), QSize(11, 5));
        QCOMPARE(load->data, QByteArrayLiteral("heif-data"));
    }
}

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
    QVERIFY(controller.isPredecodeCacheable());
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

void TestImagePresentationLoad::streamedAnimationImagesPresentFirstFrames()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/animated.png"));
    const KiriView::ImageLoadSession session = loadSession(imageUrl);

    {
        KiriView::ImagePresentationController controller = presentationController(this);
        controller.setViewportSize(QSizeF(200.0, 100.0));
        KiriView::DecodedImage decoded = KiriView::ApngAnimationImage {
            testImage(QSize(13, 7)),
            QByteArrayLiteral("apng-data"),
        };

        const KiriView::ImagePresentationLoadResult result
            = KiriView::presentDecodedImageLoad(controller, session, std::move(decoded),
                KiriView::ImagePresentationAnimationHandling::FirstFrameOnly);

        QVERIFY(result.presented);
        QCOMPARE(result.imageSize, QSize(13, 7));
        QCOMPARE(controller.imageSize(), QSize(13, 7));
        QVERIFY(!controller.isPredecodeCacheable());
    }

    {
        KiriView::ImagePresentationController controller = presentationController(this);
        controller.setViewportSize(QSizeF(200.0, 100.0));
        KiriView::DecodedImage decoded = KiriView::HeifSequenceAnimationImage {
            testImage(QSize(11, 5)),
            QByteArrayLiteral("heif-data"),
        };

        const KiriView::ImagePresentationLoadResult result
            = KiriView::presentDecodedImageLoad(controller, session, std::move(decoded),
                KiriView::ImagePresentationAnimationHandling::FirstFrameOnly);

        QVERIFY(result.presented);
        QCOMPARE(result.imageSize, QSize(11, 5));
        QCOMPARE(controller.imageSize(), QSize(11, 5));
        QVERIFY(!controller.isPredecodeCacheable());
    }
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
