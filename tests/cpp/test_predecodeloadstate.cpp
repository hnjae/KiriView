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
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::testImage;

kiriview::DisplayedPredecodeImage displayedImage(
    const QUrl &url, qreal firstDisplayPixelsPerSourcePixel = 0.0)
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
    const QUrl &displayedUrl, std::vector<QUrl> urls, quint64 generation = 7)
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

kiriview::PredecodeActiveLoads activeLoads(std::vector<QUrl> urls)
{
    return kiriview::PredecodeActiveLoads::fromUrls(std::move(urls));
}

kiriview::PredecodeLoadState loadState() { return kiriview::PredecodeLoadState(1024 * 1024); }
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
    kiriview::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }));

    const std::optional<kiriview::PredecodedImage> displayed
        = state.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(displayed->displayImage.displayPixelsPerSourcePixel, 0.5);

    const std::optional<kiriview::PredecodeLoadStart> load
        = state.takeNextLoad(kiriview::PredecodeActiveLoads {});
    QVERIFY(load.has_value());
    QCOMPARE(load->request.id(), quint64(7));
    QCOMPARE(load->request.imageUrl(), nextUrl);
    QCOMPARE(load->request.firstDisplay().physicalViewportSize, QSize(640, 480));
    QVERIFY(!state.takeNextLoad(activeLoads({ nextUrl })).has_value());
}

void TestPredecodeLoadState::activeLoadSnapshotIsTheAdmissionInput()
{
    kiriview::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    const QUrl previousUrl = indexedImageUrl(0);
    kiriview::PredecodeLoadWindow window
        = loadWindow(displayedUrl, { displayedUrl, nextUrl, previousUrl });
    window.parallelLimit = 2;

    state.startWindow(std::move(window));

    const std::optional<kiriview::PredecodeLoadStart> load
        = state.takeNextLoad(activeLoads({ nextUrl }));
    QVERIFY(load.has_value());
    QCOMPARE(load->request.imageUrl(), previousUrl);
    QVERIFY(!state.takeNextLoad(activeLoads({ nextUrl, previousUrl })).has_value());
}

void TestPredecodeLoadState::replacingWindowClearsQueuedLoadsAndUsesNextGeneration()
{
    kiriview::PredecodeLoadState state = loadState();
    const QUrl staleDisplayedUrl = indexedImageUrl(1);
    const QUrl staleNextUrl = indexedImageUrl(2);
    const QUrl displayedUrl = indexedImageUrl(3);
    const QUrl nextUrl = indexedImageUrl(4);

    state.startWindow(loadWindow(staleDisplayedUrl, { staleDisplayedUrl, staleNextUrl }));
    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }, 8));

    const std::optional<kiriview::PredecodeLoadStart> load
        = state.takeNextLoad(kiriview::PredecodeActiveLoads {});
    QVERIFY(load.has_value());
    QCOMPARE(load->request.id(), quint64(8));
    QCOMPARE(load->request.imageUrl(), nextUrl);
    QVERIFY(!state.findPredecodedImage(staleNextUrl).has_value());
    QVERIFY(state.findPredecodedImage(displayedUrl).has_value());
}

void TestPredecodeLoadState::cancelBackgroundWorkKeepsDisplayedCacheButDropsQueuedLoads()
{
    kiriview::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl, nextUrl }));
    state.cancelBackgroundWork();

    QVERIFY(!state.takeNextLoad(kiriview::PredecodeActiveLoads {}).has_value());
    QVERIFY(state.findPredecodedImage(displayedUrl).has_value());
}

void TestPredecodeLoadState::findPredecodedImageDoesNotConsumeCachedImage()
{
    kiriview::PredecodeLoadState state = loadState();
    const QUrl displayedUrl = indexedImageUrl(1);

    state.startWindow(loadWindow(displayedUrl, { displayedUrl }));

    const std::optional<kiriview::PredecodedImage> firstLookup
        = state.findPredecodedImage(displayedUrl);
    const std::optional<kiriview::PredecodedImage> secondLookup
        = state.findPredecodedImage(displayedUrl);

    QVERIFY(firstLookup.has_value());
    QVERIFY(secondLookup.has_value());
    QCOMPARE(secondLookup->location.imageUrl(), displayedUrl);
    QCOMPARE(secondLookup->displayImage.displayPixelsPerSourcePixel,
        firstLookup->displayImage.displayPixelsPerSourcePixel);
}

QTEST_GUILESS_MAIN(TestPredecodeLoadState)

#include "test_predecodeloadstate.moc"
