// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/mediapredecodecoordinator.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::ManualPowerSaverMonitor;
using kiriview::TestSupport::ManualTimerScheduler;
using kiriview::TestSupport::powerSaverProviderFor;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

std::vector<kiriview::DirectMediaNavigationCandidate> mixedDirectMediaNavigationCandidates()
{
    return {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/00.png"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.mp4"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/02.png"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };
}

kiriview::DisplayedPredecodeImage displayedImage(const QUrl &url)
{
    return kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(testImage()),
    };
}

kiriview::PowerSaverProvider noOpPowerSaverProvider()
{
    return kiriview::PowerSaverProvider {
        [](QObject *, kiriview::PowerSaverChangedCallback) {
            return std::unique_ptr<kiriview::PowerSaverStateMonitor>();
        },
    };
}

kiriview::MediaPredecodeCoordinator createCoordinator(QObject *parent,
    ManualImageDataLoader &dataLoader, kiriview::PowerSaverProvider powerSaverProvider,
    kiriview::TimerScheduler timerScheduler = {})
{
    return kiriview::MediaPredecodeCoordinator(parent,
        kiriview::MediaPredecodeDependencies {
            imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()),
            std::move(powerSaverProvider),
            testCacheByteBudget,
            std::move(timerScheduler),
        });
}

kiriview::MediaPredecodeCoordinator createCoordinator(
    QObject *parent, ManualImageDataLoader &dataLoader)
{
    return createCoordinator(parent, dataLoader, noOpPowerSaverProvider());
}
}

class TestMediaPredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void videoCursorKeepsImageCacheAndLoadsAdjacentImages();
    void powerSaverSuppressesLoadsButRetainsDisplayedImages();
    void powerSaverReschedulesVideoCursorWithoutDisplayedImages();
    void invalidScheduleClearsSuppressedDirectMediaNavigationCandidates();
};

void TestMediaPredecodeCoordinator::videoCursorKeepsImageCacheAndLoadsAdjacentImages()
{
    ManualImageDataLoader dataLoader;
    kiriview::MediaPredecodeCoordinator coordinator = createCoordinator(this, dataLoader);

    const QUrl displayedUrl = localUrl(QStringLiteral("/media/00.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl laterUrl = localUrl(QStringLiteral("/media/03.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates
        = mixedDirectMediaNavigationCandidates();

    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {
        displayedUrl,
        candidates,
        { displayedImage(displayedUrl) },
    });

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(nextUrl, QByteArrayLiteral("next")));
    QTRY_VERIFY(coordinator.findPredecodedImage(nextUrl).has_value());

    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {
        videoUrl,
        candidates,
        {},
    });

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, laterUrl);
    QVERIFY(dataLoader.backLoad().url != videoUrl);
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QVERIFY(coordinator.findPredecodedImage(nextUrl).has_value());
}

void TestMediaPredecodeCoordinator::powerSaverSuppressesLoadsButRetainsDisplayedImages()
{
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    ManualTimerScheduler timerScheduler;
    kiriview::MediaPredecodeCoordinator coordinator = createCoordinator(this, dataLoader,
        powerSaverProviderFor(powerSaverMonitor, true), timerScheduler.scheduler());
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = localUrl(QStringLiteral("/media/00.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    timerScheduler.advanceTo(1000);
    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {
        displayedUrl,
        mixedDirectMediaNavigationCandidates(),
        { displayedImage(displayedUrl) },
    });

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    timerScheduler.advanceTo(1200);
    powerSaverMonitor->setPowerSaverEnabled(false);

    QVERIFY(timerScheduler.timerAt(0).active());
    timerScheduler.timerAt(0).fire();

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestMediaPredecodeCoordinator::powerSaverReschedulesVideoCursorWithoutDisplayedImages()
{
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    ManualTimerScheduler timerScheduler;
    kiriview::MediaPredecodeCoordinator coordinator = createCoordinator(this, dataLoader,
        powerSaverProviderFor(powerSaverMonitor, true), timerScheduler.scheduler());
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl videoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    timerScheduler.advanceTo(1000);
    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {
        videoUrl,
        mixedDirectMediaNavigationCandidates(),
        {},
    });

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    timerScheduler.advanceTo(1200);
    powerSaverMonitor->setPowerSaverEnabled(false);

    QVERIFY(timerScheduler.timerAt(0).active());
    timerScheduler.timerAt(0).fire();

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
}

void TestMediaPredecodeCoordinator::invalidScheduleClearsSuppressedDirectMediaNavigationCandidates()
{
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    ManualTimerScheduler timerScheduler;
    kiriview::MediaPredecodeCoordinator coordinator = createCoordinator(this, dataLoader,
        powerSaverProviderFor(powerSaverMonitor, true), timerScheduler.scheduler());
    QVERIFY(powerSaverMonitor != nullptr);

    timerScheduler.advanceTo(1000);
    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {
        localUrl(QStringLiteral("/media/01.mp4")),
        mixedDirectMediaNavigationCandidates(),
        {},
    });
    coordinator.schedule(kiriview::MediaPredecodeCoordinator::Context {});

    powerSaverMonitor->setPowerSaverEnabled(false);

    QVERIFY(!timerScheduler.timerAt(0).active());
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
}

QTEST_GUILESS_MAIN(TestMediaPredecodeCoordinator)

#include "test_mediapredecodecoordinator.moc"
