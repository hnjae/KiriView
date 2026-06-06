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
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::testImage;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

KiriView::DisplayedPredecodeImage displayedImage(const QUrl &url)
{
    return KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        true,
        staticDisplayTestImagePayload(testImage()),
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

class ManualRuntimeTimer final : public KiriView::RuntimeTimerHandle
{
public:
    ManualRuntimeTimer(int intervalMsec, KiriView::RuntimeTimerCallback callback)
        : m_intervalMsec(intervalMsec)
        , m_callback(std::move(callback))
    {
    }

    int intervalMsec() const { return m_intervalMsec; }
    bool active() const { return m_active; }

    void start() override { m_active = true; }
    void stop() override { m_active = false; }

    void fire()
    {
        if (!m_active || !m_callback) {
            return;
        }

        m_active = false;
        m_callback();
    }

private:
    int m_intervalMsec = 0;
    KiriView::RuntimeTimerCallback m_callback;
    bool m_active = false;
};

class ManualTimerScheduler
{
public:
    KiriView::TimerScheduler scheduler()
    {
        return KiriView::TimerScheduler {
            [this]() { return m_currentMsec; },
            [this](QObject *, int intervalMsec, KiriView::RuntimeTimerCallback callback)
                -> std::unique_ptr<KiriView::RuntimeTimerHandle> {
                auto timer
                    = std::make_unique<ManualRuntimeTimer>(intervalMsec, std::move(callback));
                m_timers.push_back(timer.get());
                return timer;
            },
        };
    }

    void advanceTo(qint64 monotonicMsec) { m_currentMsec = monotonicMsec; }
    std::size_t timerCount() const { return m_timers.size(); }
    ManualRuntimeTimer &timerAt(std::size_t index) { return *m_timers.at(index); }

private:
    qint64 m_currentMsec = 0;
    std::vector<ManualRuntimeTimer *> m_timers;
};
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
    KiriView::PredecodeLoadController loadController(
        this, KiriView::ImageDecodeDependencies {}, testCacheByteBudget);
    int startCount = 0;
    std::optional<KiriView::PredecodePendingSchedule> capturedSchedule;
    ManualTimerScheduler timerScheduler;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const KiriView::PredecodePendingSchedule &schedule) {
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
    KiriView::PredecodeLoadController loadController(
        this, KiriView::ImageDecodeDependencies {}, testCacheByteBudget);
    ManualTimerScheduler timerScheduler;
    int startCount = 0;
    std::optional<KiriView::PredecodePendingSchedule> capturedSchedule;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const KiriView::PredecodePendingSchedule &schedule) {
            ++startCount;
            capturedSchedule = schedule;
        },
        {}, noOpPowerSaverProvider(), timerScheduler.scheduler());

    const QUrl displayedUrl = indexedImageUrl(4);
    timerScheduler.advanceTo(1000);
    runtime.schedule(scheduleContext(displayedUrl));

    QCOMPARE(timerScheduler.timerCount(), std::size_t(2));
    QCOMPARE(timerScheduler.timerAt(0).intervalMsec(), KiriView::predecodeDebounceMsec());
    QVERIFY(timerScheduler.timerAt(0).active());
    QCOMPARE(startCount, 0);

    timerScheduler.timerAt(0).fire();

    QCOMPARE(startCount, 1);
    QVERIFY(capturedSchedule.has_value());
    QCOMPARE(capturedSchedule->context.currentLocation.imageUrl(), displayedUrl);
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
    ManualTimerScheduler timerScheduler;
    int startCount = 0;
    std::optional<KiriView::PredecodePendingSchedule> capturedSchedule;
    KiriView::PredecodeScheduleRuntime runtime(
        this, loadController,
        [&startCount, &capturedSchedule](const KiriView::PredecodePendingSchedule &schedule) {
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
