// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/predecodeloadcontroller.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

kiriview::DisplayedPredecodeImage displayedImage(
    const QUrl& url, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    const kiriview::DisplayImageQuality quality = firstDisplayPixelsPerSourcePixel > 0.0
        ? kiriview::DisplayImageQuality::FirstDisplay
        : kiriview::DisplayImageQuality::Exact;
    return kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(
            testImage(), testImage(), firstDisplayPixelsPerSourcePixel, quality),
    };
}

kiriview::PredecodeLoadWindow loadWindow(
    const QUrl& displayedUrl, std::vector<QUrl> urls, quint64 generation = 7)
{
    return kiriview::PredecodeLoadWindow {
        displayedUrl,
        kiriview::OpenedCollectionScopeLocation::none(),
        std::move(urls),
        { displayedImage(displayedUrl, 0.5) },
        kiriview::ImageFirstDisplayDecodeContext { QSize(640, 480) },
        generation,
        1,
    };
}
}

class TestPredecodeLoadController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowLoadsCacheDisplayedImageAndPumpQueuedDecodes();
    void parallelLimitStartsMultipleWindowLoads();
    void startWindowLoadsReplacesActiveGeneration();
    void cancelBackgroundWorkSuppressesStaleDecode();
};

void TestPredecodeLoadController::windowLoadsCacheDisplayedImageAndPumpQueuedDecodes()
{
    ManualImageDataLoader dataLoader;
    kiriview::PredecodeLoadController controller(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    const QUrl previousUrl = indexedImageUrl(0);

    controller.startWindowLoads(loadWindow(displayedUrl, { displayedUrl, nextUrl, previousUrl }));

    const std::optional<kiriview::PredecodedImage> displayed
        = controller.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(displayed->displayImage.displayPixelsPerSourcePixel, 0.5);

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));

    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));
    QTRY_VERIFY(controller.findPredecodedImage(nextUrl).has_value());
    const std::optional<kiriview::PredecodedImage> next = controller.findPredecodedImage(nextUrl);
    QCOMPARE(next->displayImage.sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(next->displayImage.originalSize, testImage().size());
    QCOMPARE(next->displayImage.image.size(), testImage().size());
    QCOMPARE(next->displayImage.quality, kiriview::DisplayImageQuality::Exact);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
}

void TestPredecodeLoadController::parallelLimitStartsMultipleWindowLoads()
{
    ManualImageDataLoader dataLoader;
    kiriview::PredecodeLoadController controller(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    const QUrl previousUrl = indexedImageUrl(0);
    kiriview::PredecodeLoadWindow window
        = loadWindow(displayedUrl, { displayedUrl, nextUrl, previousUrl });
    window.parallelLimit = 2;

    controller.startWindowLoads(std::move(window));

    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
}

void TestPredecodeLoadController::startWindowLoadsReplacesActiveGeneration()
{
    ManualImageDataLoader dataLoader;
    kiriview::PredecodeLoadController controller(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);
    const QUrl staleDisplayedUrl = indexedImageUrl(1);
    const QUrl staleNextUrl = indexedImageUrl(2);
    const QUrl displayedUrl = indexedImageUrl(3);
    const QUrl nextUrl = indexedImageUrl(4);

    controller.startWindowLoads(loadWindow(staleDisplayedUrl, { staleDisplayedUrl, staleNextUrl }));
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, staleNextUrl);

    controller.startWindowLoads(loadWindow(displayedUrl, { displayedUrl, nextUrl }, 8));

    QVERIFY(dataLoader.frontLoad().canceled);
    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextUrl);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QTest::qWait(50);
    QVERIFY(!controller.findPredecodedImage(staleNextUrl).has_value());

    dataLoader.finishBackLoad(QByteArrayLiteral("next"));
    QTRY_VERIFY(controller.findPredecodedImage(nextUrl).has_value());
}

void TestPredecodeLoadController::cancelBackgroundWorkSuppressesStaleDecode()
{
    ManualImageDataLoader dataLoader;
    kiriview::PredecodeLoadController controller(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);

    controller.startWindowLoads(loadWindow(displayedUrl, { displayedUrl, nextUrl }));
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));

    controller.cancelBackgroundWork();
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QTest::qWait(50);
    QVERIFY(!controller.findPredecodedImage(nextUrl).has_value());
    QVERIFY(controller.findPredecodedImage(displayedUrl).has_value());
}

QTEST_GUILESS_MAIN(TestPredecodeLoadController)

#include "test_predecodeloadcontroller.moc"
