// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/mediapredecodedependencies.h"

#include "decoding/decodedimageresult.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <memory>
#include <utility>

namespace {
class FakePowerSaverMonitor final : public KiriView::PowerSaverStateMonitor
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
    void explicitDependenciesArePreserved();
};

void TestMediaPredecodeDependencies::defaultsFillRuntimeProvidersAndBudget()
{
    KiriView::MediaPredecodeDependencies dependencies
        = KiriView::resolveMediaPredecodeDependencies({});

    QVERIFY(dependencies.imageDecode.dataLoader);
    QVERIFY(dependencies.imageDecode.dataDecoder);
    QVERIFY(dependencies.powerSaver.monitor);
    QVERIFY(dependencies.cacheByteBudget > 0);
    QVERIFY(dependencies.cacheByteBudget <= KiriView::predecodeCachePreferredByteBudget());
}

void TestMediaPredecodeDependencies::explicitDependenciesArePreserved()
{
    int dataLoadCount = 0;
    int dataDecodeCount = 0;
    int powerSaverMonitorCount = 0;
    QByteArray decodedData;

    KiriView::MediaPredecodeDependencyOverrides overrides;
    overrides.imageDecode.dataLoader
        = [&dataLoadCount](QObject *, KiriView::ImageDecodeRequest,
              KiriView::ImageDataCallback callback, KiriView::ErrorCallback) {
              ++dataLoadCount;
              callback(QByteArrayLiteral("custom media predecode data"));
              return KiriView::ImageIoJob();
          };
    overrides.imageDecode.dataDecoder = [&dataDecodeCount, &decodedData](const QByteArray &data,
                                            const KiriView::ImageDecodeRequest &) {
        ++dataDecodeCount;
        decodedData = data;
        return KiriView::failedDecodedImageResult(QStringLiteral("decoded by override"));
    };
    overrides.powerSaver.monitor
        = [&powerSaverMonitorCount](QObject *, KiriView::PowerSaverChangedCallback) {
              ++powerSaverMonitorCount;
              return std::make_unique<FakePowerSaverMonitor>();
          };
    overrides.cacheBudgetRequest.predecodeCacheByteBudget = 4096;

    KiriView::MediaPredecodeDependencies dependencies
        = KiriView::resolveMediaPredecodeDependencies(std::move(overrides));

    QByteArray loadedData;
    dependencies.imageDecode.dataLoader(nullptr, KiriView::ImageDecodeRequest(),
        [&loadedData](QByteArray data) { loadedData = std::move(data); }, {});
    const KiriView::DecodedImageResult result
        = dependencies.imageDecode.dataDecoder(loadedData, KiriView::ImageDecodeRequest());
    std::unique_ptr<KiriView::PowerSaverStateMonitor> monitor
        = dependencies.powerSaver.monitor(nullptr, {});

    QCOMPARE(dataLoadCount, 1);
    QCOMPARE(dataDecodeCount, 1);
    QCOMPARE(decodedData, QByteArrayLiteral("custom media predecode data"));
    QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(powerSaverMonitorCount, 1);
    QVERIFY(monitor);
    QVERIFY(monitor->powerSaverEnabled());
    QCOMPARE(dependencies.cacheByteBudget, qsizetype(4096));
}

QTEST_GUILESS_MAIN(TestMediaPredecodeDependencies)

#include "test_mediapredecodedependencies.moc"
