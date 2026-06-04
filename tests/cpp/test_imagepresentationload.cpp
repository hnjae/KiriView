// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationload.h"

#include "image_test_support.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "rendering/displayimagestore.h"
#include "rendering/imagerendering.h"

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticDecodedTestImage;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

constexpr qsizetype testPredecodeCacheByteBudget = 1024 * 1024;
constexpr qsizetype testStaticTileCacheByteBudget = 512 * 1024;

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
    void staticDecodedPredecodeCacheabilityUsesInjectedBudget();
    void animationHandlingControlsPlannedEffects();
    void staticDecodedImagesAreAppliedToPresentation();
    void staticDecodedImagesPublishProviderSourceAndKeepCompatibilitySurface();
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
    QVERIFY(!displaySource.loadAcknowledgmentRequired);

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
