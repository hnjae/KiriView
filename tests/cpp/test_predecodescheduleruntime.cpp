// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/predecodescheduleruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>

namespace {
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

KiriView::DisplayedPredecodeImage displayedImage(const QUrl &url)
{
    return KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        true,
        staticTestImagePayload(testImage()),
    };
}

KiriView::PredecodeScheduleContext scheduleContext(const QUrl &url)
{
    return KiriView::PredecodeScheduleContext {
        KiriView::DisplayedImageLocation::fromUrl(url),
        { displayedImage(url) },
        {},
        1,
        {},
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
}

class TestPredecodeScheduleRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImagesAndStartsAdjacentAfterDebounce();
    void invalidScheduleCancelsDomainBackgroundWork();
    void powerSaverSuppressesAndReschedulesPendingPredecode();
};

void TestPredecodeScheduleRuntime::scheduleCachesDisplayedImagesAndStartsAdjacentAfterDebounce()
{
    KiriView::PredecodeLoadController loadController(
        this, KiriView::ImageDecodeDependencies {}, testCacheByteBudget);
    int startCount = 0;
    std::optional<KiriView::PredecodePendingSchedule> capturedSchedule;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const KiriView::PredecodePendingSchedule &schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        noOpPowerSaverProvider());

    const QUrl displayedUrl = indexedImageUrl(3);
    runtime.schedule(scheduleContext(displayedUrl));

    QVERIFY(loadController.findPredecodedImage(displayedUrl).has_value());
    QTRY_COMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
    QVERIFY(runtime.accepts(capturedSchedule->generation));
}

void TestPredecodeScheduleRuntime::invalidScheduleCancelsDomainBackgroundWork()
{
    KiriView::PredecodeLoadController loadController(
        this, KiriView::ImageDecodeDependencies {}, testCacheByteBudget);
    int cancelCount = 0;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController, [](const KiriView::PredecodePendingSchedule &) {},
        [&cancelCount]() { ++cancelCount; }, noOpPowerSaverProvider());

    runtime.schedule({});

    QCOMPARE(cancelCount, 1);
}

void TestPredecodeScheduleRuntime::powerSaverSuppressesAndReschedulesPendingPredecode()
{
    KiriView::PredecodeLoadController loadController(
        this, KiriView::ImageDecodeDependencies {}, testCacheByteBudget);
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    int startCount = 0;
    std::optional<KiriView::PredecodePendingSchedule> capturedSchedule;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const KiriView::PredecodePendingSchedule &schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = indexedImageUrl(5);
    runtime.schedule(scheduleContext(displayedUrl));

    QVERIFY(loadController.findPredecodedImage(displayedUrl).has_value());
    QTest::qWait(250);
    QCOMPARE(startCount, 0);

    powerSaverMonitor->setPowerSaverEnabled(false);

    QTRY_COMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
}

QTEST_GUILESS_MAIN(TestPredecodeScheduleRuntime)

#include "test_predecodescheduleruntime.moc"
