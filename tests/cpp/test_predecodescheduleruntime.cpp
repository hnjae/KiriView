// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/predecodescheduleruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::ManualPowerSaverMonitor;
using kiriview::TestSupport::ManualTimerScheduler;
using kiriview::TestSupport::powerSaverProviderFor;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

kiriview::DisplayedPredecodeImage displayedImage(const QUrl& url)
{
    return kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(testImage()),
    };
}

kiriview::PredecodeScheduleContext scheduleContext(const QUrl& url)
{
    return kiriview::PredecodeScheduleContext {
        kiriview::DisplayedImageLocation::fromUrl(url),
        { displayedImage(url) },
        {},
        1,
        {},
    };
}

kiriview::PowerSaverProvider noOpPowerSaverProvider()
{
    return kiriview::PowerSaverProvider {
        [](QObject*, kiriview::PowerSaverChangedCallback) {
            return std::unique_ptr<kiriview::PowerSaverStateMonitor>();
        },
    };
}

}

class TestPredecodeScheduleRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImagesAndStartsAdjacentAfterDebounce();
    void manualTimerSchedulerFiresDebouncedPredecode();
    void invalidScheduleCancelsDomainBackgroundWork();
    void powerSaverSuppressesAndReschedulesPendingPredecode();
};

void TestPredecodeScheduleRuntime::scheduleCachesDisplayedImagesAndStartsAdjacentAfterDebounce()
{
    kiriview::PredecodeLoadController loadController(
        this, kiriview::ImageDecodeDependencies {}, testCacheByteBudget);
    int startCount = 0;
    std::optional<kiriview::PredecodePendingSchedule> capturedSchedule;
    ManualTimerScheduler timerScheduler;
    kiriview::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const kiriview::PredecodePendingSchedule& schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        {}, noOpPowerSaverProvider(), timerScheduler.scheduler());

    const QUrl displayedUrl = indexedImageUrl(3);
    timerScheduler.advanceTo(1000);
    runtime.schedule(scheduleContext(displayedUrl));

    QVERIFY(loadController.findPredecodedImage(displayedUrl).has_value());
    QCOMPARE(startCount, 0);
    QVERIFY(timerScheduler.timerAt(0).active());

    timerScheduler.timerAt(0).fire();

    QCOMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
    QVERIFY(runtime.accepts(capturedSchedule->generation));
}

void TestPredecodeScheduleRuntime::manualTimerSchedulerFiresDebouncedPredecode()
{
    kiriview::PredecodeLoadController loadController(
        this, kiriview::ImageDecodeDependencies {}, testCacheByteBudget);
    ManualTimerScheduler timerScheduler;
    int startCount = 0;
    std::optional<kiriview::PredecodePendingSchedule> capturedSchedule;
    kiriview::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const kiriview::PredecodePendingSchedule& schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        {}, noOpPowerSaverProvider(), timerScheduler.scheduler());

    const QUrl displayedUrl = indexedImageUrl(4);
    timerScheduler.advanceTo(1000);
    runtime.schedule(scheduleContext(displayedUrl));

    QCOMPARE(timerScheduler.timerCount(), std::size_t(2));
    QCOMPARE(timerScheduler.timerAt(0).intervalMsec(), kiriview::predecodeDebounceMsec());
    QVERIFY(timerScheduler.timerAt(0).active());
    QCOMPARE(startCount, 0);

    timerScheduler.timerAt(0).fire();

    QCOMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
}

void TestPredecodeScheduleRuntime::invalidScheduleCancelsDomainBackgroundWork()
{
    kiriview::PredecodeLoadController loadController(
        this, kiriview::ImageDecodeDependencies {}, testCacheByteBudget);
    int cancelCount = 0;
    kiriview::PredecodeScheduleRuntime runtime(
        this, loadController, [](const kiriview::PredecodePendingSchedule&) {},
        [&cancelCount]() { ++cancelCount; }, noOpPowerSaverProvider());

    runtime.schedule({});

    QCOMPARE(cancelCount, 1);
}

void TestPredecodeScheduleRuntime::powerSaverSuppressesAndReschedulesPendingPredecode()
{
    kiriview::PredecodeLoadController loadController(
        this, kiriview::ImageDecodeDependencies {}, testCacheByteBudget);
    ManualPowerSaverMonitor* powerSaverMonitor = nullptr;
    ManualTimerScheduler timerScheduler;
    int startCount = 0;
    std::optional<kiriview::PredecodePendingSchedule> capturedSchedule;
    kiriview::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const kiriview::PredecodePendingSchedule& schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        {}, powerSaverProviderFor(powerSaverMonitor, true), timerScheduler.scheduler());
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = indexedImageUrl(5);
    timerScheduler.advanceTo(1000);
    runtime.schedule(scheduleContext(displayedUrl));

    QVERIFY(loadController.findPredecodedImage(displayedUrl).has_value());
    QCOMPARE(startCount, 0);
    QVERIFY(!timerScheduler.timerAt(0).active());

    timerScheduler.advanceTo(1200);
    powerSaverMonitor->setPowerSaverEnabled(false);

    QVERIFY(timerScheduler.timerAt(0).active());
    QCOMPARE(startCount, 0);

    timerScheduler.timerAt(0).fire();

    QCOMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleRuntime)

#include "test_predecodescheduleruntime.moc"
