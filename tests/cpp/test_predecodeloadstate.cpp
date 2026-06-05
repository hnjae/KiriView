// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/predecodeloadstate.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::DisplayedPredecodeImage displayedImage(
    const QUrl &url, KiriView::StaticImageDisplayHints displayHints = {})
{
    const KiriView::DisplayImageQuality quality
        = displayHints.firstDisplayPixelsPerSourcePixel > 0.0
        ? KiriView::DisplayImageQuality::FirstDisplay
        : KiriView::DisplayImageQuality::Exact;
    return KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(testImage(), testImage(), displayHints, quality),
    };
}

KiriView::PredecodeLoadWindow loadWindow(
    const QUrl &displayedUrl, std::vector<QUrl> urls, quint64 generation = 7)
{
    return KiriView::PredecodeLoadWindow {
        displayedUrl,
        KiriView::OpenedCollectionScopeLocation::none(),
        std::move(urls),
        { displayedImage(displayedUrl, KiriView::StaticImageDisplayHints { 0.5 }) },
        KiriView::ImageFirstDisplayDecodeContext { QSize(640, 480) },
        generation,
        1,
    };
}

KiriView::PredecodeActiveLoads activeLoads(std::vector<QUrl> urls)
{
    return KiriView::PredecodeActiveLoads::fromUrls(std::move(urls));
}

KiriView::PredecodeLoadState loadState() { return KiriView::PredecodeLoadState(1024 * 1024); }
}

class TestPredecodeLoadState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activeWindowBuildsDecodeRequestsFromCanonicalContext();
    void activeLoadSnapshotIsTheAdmissionInput();
    void replacingWindowClearsQueuedLoadsAndUsesNextGeneration();
    void cancelBackgroundWorkKeepsDisplayedCacheButDropsQueuedLoads();
    void findPredecodedImageDoesNotConsumeCachedImage();
};

void TestPredecodeLoadState::activeWindowBuildsDecodeRequestsFromCanonicalContext()
{
    KiriView::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }));

    const std::optional<KiriView::PredecodedImage> displayed
        = state.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->displayImage.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(displayed->displayImage.displayPixelsPerSourcePixel, 0.5);

    const std::optional<KiriView::PredecodeLoadStart> load
        = state.takeNextLoad(KiriView::PredecodeActiveLoads {});
    QVERIFY(load.has_value());
    QCOMPARE(load->request.id(), quint64(7));
    QCOMPARE(load->request.imageUrl(), nextUrl);
    QCOMPARE(load->request.firstDisplay().physicalViewportSize, QSize(640, 480));
    QVERIFY(!state.takeNextLoad(activeLoads({ nextUrl })).has_value());
}

void TestPredecodeLoadState::activeLoadSnapshotIsTheAdmissionInput()
{
    KiriView::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    const QUrl previousUrl = indexedImageUrl(0);
    KiriView::PredecodeLoadWindow window
        = loadWindow(displayedUrl, { displayedUrl, nextUrl, previousUrl });
    window.parallelLimit = 2;

    state.startWindow(std::move(window));

    const std::optional<KiriView::PredecodeLoadStart> load
        = state.takeNextLoad(activeLoads({ nextUrl }));
    QVERIFY(load.has_value());
    QCOMPARE(load->request.imageUrl(), previousUrl);
    QVERIFY(!state.takeNextLoad(activeLoads({ nextUrl, previousUrl })).has_value());
}

void TestPredecodeLoadState::replacingWindowClearsQueuedLoadsAndUsesNextGeneration()
{
    KiriView::PredecodeLoadState state = loadState();
    const QUrl staleDisplayedUrl = indexedImageUrl(1);
    const QUrl staleNextUrl = indexedImageUrl(2);
    const QUrl displayedUrl = indexedImageUrl(3);
    const QUrl nextUrl = indexedImageUrl(4);

    state.startWindow(loadWindow(staleDisplayedUrl, { staleDisplayedUrl, staleNextUrl }));
    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }, 8));

    const std::optional<KiriView::PredecodeLoadStart> load
        = state.takeNextLoad(KiriView::PredecodeActiveLoads {});
    QVERIFY(load.has_value());
    QCOMPARE(load->request.id(), quint64(8));
    QCOMPARE(load->request.imageUrl(), nextUrl);
    QVERIFY(!state.findPredecodedImage(staleNextUrl).has_value());
    QVERIFY(state.findPredecodedImage(displayedUrl).has_value());
}

void TestPredecodeLoadState::cancelBackgroundWorkKeepsDisplayedCacheButDropsQueuedLoads()
{
    KiriView::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }));
    state.cancelBackgroundWork();

    QVERIFY(!state.takeNextLoad(KiriView::PredecodeActiveLoads {}).has_value());
    QVERIFY(state.findPredecodedImage(displayedUrl).has_value());
}

void TestPredecodeLoadState::findPredecodedImageDoesNotConsumeCachedImage()
{
    KiriView::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl }));

    const std::optional<KiriView::PredecodedImage> firstLookup
        = state.findPredecodedImage(displayedUrl);
    const std::optional<KiriView::PredecodedImage> secondLookup
        = state.findPredecodedImage(displayedUrl);

    QVERIFY(firstLookup.has_value());
    QVERIFY(secondLookup.has_value());
    QCOMPARE(secondLookup->location.imageUrl(), displayedUrl);
    QCOMPARE(secondLookup->displayImage.displayPixelsPerSourcePixel,
        firstLookup->displayImage.displayPixelsPerSourcePixel);
}

QTEST_GUILESS_MAIN(TestPredecodeLoadState)

#include "test_predecodeloadstate.moc"
