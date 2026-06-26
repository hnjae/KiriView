// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/mediapredecodedependencies.h"

#include "decoding/decodedimageresult.h"
#include "image_async_test_support.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <memory>
#include <utility>

namespace {
class FakePowerSaverMonitor final : public kiriview::PowerSaverStateMonitor
{
public:
    bool powerSaverEnabled() const override { return true; }
};
}

class TestMediaPredecodeDependencies : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsFillRuntimeProvidersAndBudget();
    void defaultBudgetUsesInjectedSystemMemorySnapshot();
    void explicitDependenciesArePreserved();
};

void TestMediaPredecodeDependencies::defaultsFillRuntimeProvidersAndBudget()
{
    kiriview::MediaPredecodeDependencies dependencies
        = kiriview::resolveMediaPredecodeDependencies({});

    QVERIFY(dependencies.imageDecode.dataLoader);
    QVERIFY(dependencies.imageDecode.dataDecoder);
    QVERIFY(dependencies.powerSaver.monitor);
    QVERIFY(dependencies.timerScheduler.currentMonotonicMsec);
    QVERIFY(dependencies.timerScheduler.singleShotTimer);
    QVERIFY(dependencies.cacheByteBudget > 0);
    QVERIFY(dependencies.cacheByteBudget <= kiriview::predecodeCachePreferredByteBudget());
}

void TestMediaPredecodeDependencies::defaultBudgetUsesInjectedSystemMemorySnapshot()
{
    constexpr qsizetype physicalByteSize = 1024 * 1024 * 1024;
    kiriview::MediaPredecodeDependencyOverrides overrides;
    overrides.systemMemorySnapshot = kiriview::SystemMemorySnapshot { physicalByteSize };

    kiriview::MediaPredecodeDependencies dependencies
        = kiriview::resolveMediaPredecodeDependencies(std::move(overrides));

    QCOMPARE(dependencies.cacheByteBudget, physicalByteSize / 8);
}

void TestMediaPredecodeDependencies::explicitDependenciesArePreserved()
{
    int dataLoadCount = 0;
    int dataDecodeCount = 0;
    int powerSaverMonitorCount = 0;
    int timerFactoryCount = 0;
    QByteArray decodedData;

    kiriview::MediaPredecodeDependencyOverrides overrides;
    overrides.imageDecode.dataLoader
        = [&dataLoadCount](QObject*, kiriview::ImageDecodeRequest,
              kiriview::ImageDataCallback callback, kiriview::ErrorCallback) {
              ++dataLoadCount;
              callback(QByteArrayLiteral("custom media predecode data"));
              return kiriview::ImageIoJob();
          };
    overrides.imageDecode.dataDecoder = [&dataDecodeCount, &decodedData](const QByteArray& data,
                                            const kiriview::ImageDecodeRequest&) {
        ++dataDecodeCount;
        decodedData = data;
        return kiriview::failedDecodedImageResult(QStringLiteral("decoded by override"));
    };
    overrides.powerSaver.monitor
        = [&powerSaverMonitorCount](QObject*, kiriview::PowerSaverChangedCallback) {
              ++powerSaverMonitorCount;
              return std::make_unique<FakePowerSaverMonitor>();
          };
    overrides.timerScheduler.currentMonotonicMsec = []() { return 4242; };
    overrides.timerScheduler.singleShotTimer = [&timerFactoryCount](QObject*, int intervalMsec,
                                                   kiriview::RuntimeTimerCallback callback) {
        ++timerFactoryCount;
        return std::make_unique<kiriview::TestSupport::ManualRuntimeTimer>(
            intervalMsec, std::move(callback));
    };
    overrides.cacheBudgetRequest.predecodeCacheByteBudget = 4096;

    kiriview::MediaPredecodeDependencies dependencies
        = kiriview::resolveMediaPredecodeDependencies(std::move(overrides));

    QByteArray loadedData;
    dependencies.imageDecode.dataLoader(nullptr, kiriview::ImageDecodeRequest(),
        [&loadedData](QByteArray data) { loadedData = std::move(data); }, {});
    const kiriview::DecodedImageResult result
        = dependencies.imageDecode.dataDecoder(loadedData, kiriview::ImageDecodeRequest());
    std::unique_ptr<kiriview::PowerSaverStateMonitor> monitor
        = dependencies.powerSaver.monitor(nullptr, {});
    std::unique_ptr<kiriview::RuntimeTimerHandle> timer
        = dependencies.timerScheduler.singleShotTimer(nullptr, 25, {});

    QCOMPARE(dataLoadCount, 1);
    QCOMPARE(dataDecodeCount, 1);
    QCOMPARE(decodedData, QByteArrayLiteral("custom media predecode data"));
    QVERIFY(kiriview::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(powerSaverMonitorCount, 1);
    QVERIFY(monitor);
    QVERIFY(monitor->powerSaverEnabled());
    QCOMPARE(dependencies.timerScheduler.currentMonotonicMsec(), qint64(4242));
    QCOMPARE(timerFactoryCount, 1);
    QVERIFY(timer);
    QCOMPARE(dependencies.cacheByteBudget, qsizetype(4096));
}

QTEST_GUILESS_MAIN(TestMediaPredecodeDependencies)

#include "test_mediapredecodedependencies.moc"
