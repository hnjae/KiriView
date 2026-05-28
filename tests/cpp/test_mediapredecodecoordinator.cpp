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
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

std::vector<KiriView::DirectMediaNavigationCandidate> mixedDirectMediaNavigationCandidates()
{
    return {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/00.png"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.mp4"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/02.png"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };
}

KiriView::DisplayedPredecodeImage displayedImage(const QUrl &url)
{
    return KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        true,
        staticTestImagePayload(testImage()),
    };
}

KiriView::PowerSaverProvider noOpPowerSaverProvider()
{
    return KiriView::PowerSaverProvider {
        [](QObject *, KiriView::PowerSaverChangedCallback) {
            return std::unique_ptr<KiriView::PowerSaverStateMonitor>();
        },
    };
}

KiriView::MediaPredecodeCoordinator createCoordinator(QObject *parent,
    ManualImageDataLoader &dataLoader, KiriView::PowerSaverProvider powerSaverProvider)
{
    return KiriView::MediaPredecodeCoordinator(parent,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()),
        std::move(powerSaverProvider), testCacheByteBudget);
}

KiriView::MediaPredecodeCoordinator createCoordinator(
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
    KiriView::MediaPredecodeCoordinator coordinator = createCoordinator(this, dataLoader);

    const QUrl displayedUrl = localUrl(QStringLiteral("/media/00.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl laterUrl = localUrl(QStringLiteral("/media/03.png"));
    const std::vector<KiriView::DirectMediaNavigationCandidate> candidates
        = mixedDirectMediaNavigationCandidates();

    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {
        displayedUrl,
        candidates,
        { displayedImage(displayedUrl) },
    });

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(nextUrl, QByteArrayLiteral("next")));
    QTRY_VERIFY(coordinator.findPredecodedImage(nextUrl).has_value());

    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {
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
    KiriView::MediaPredecodeCoordinator coordinator
        = createCoordinator(this, dataLoader, powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = localUrl(QStringLiteral("/media/00.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {
        displayedUrl,
        mixedDirectMediaNavigationCandidates(),
        { displayedImage(displayedUrl) },
    });

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    powerSaverMonitor->setPowerSaverEnabled(false);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestMediaPredecodeCoordinator::powerSaverReschedulesVideoCursorWithoutDisplayedImages()
{
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    KiriView::MediaPredecodeCoordinator coordinator
        = createCoordinator(this, dataLoader, powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl videoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {
        videoUrl,
        mixedDirectMediaNavigationCandidates(),
        {},
    });

    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    powerSaverMonitor->setPowerSaverEnabled(false);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
}

void TestMediaPredecodeCoordinator::invalidScheduleClearsSuppressedDirectMediaNavigationCandidates()
{
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    KiriView::MediaPredecodeCoordinator coordinator
        = createCoordinator(this, dataLoader, powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);

    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {
        localUrl(QStringLiteral("/media/01.mp4")),
        mixedDirectMediaNavigationCandidates(),
        {},
    });
    coordinator.schedule(KiriView::MediaPredecodeCoordinator::Context {});

    powerSaverMonitor->setPowerSaverEnabled(false);

    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
}

QTEST_GUILESS_MAIN(TestMediaPredecodeCoordinator)

#include "test_mediapredecodecoordinator.moc"
