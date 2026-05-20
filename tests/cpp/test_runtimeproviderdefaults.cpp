// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "document/filedeletion.h"
#include "navigation/imagecandidateprovider.h"
#include "powersaverprovider.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>

class TestRuntimeProviderDefaults : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides();
    void decodeDependencyDefaultsFillMissingFunctionsAndPreserveOverrides();
    void fileOperationDefaultFillsMissingProviderAndPreservesOverride();
    void powerSaverDefaultFillsMissingProviderAndPreservesOverride();
};

void TestRuntimeProviderDefaults::candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides()
{
    int directoryLoadCount = 0;
    int directoryChangeSubscriptionCount = 0;
    KiriView::ImageNavigationCandidateProvider provider;
    provider.directoryImages = [&directoryLoadCount](QObject *, QUrl,
                                   KiriView::ImageCandidatesCallback, KiriView::ErrorCallback) {
        ++directoryLoadCount;
        return KiriView::ImageIoJob();
    };
    provider.directoryImageChanges
        = [&directoryChangeSubscriptionCount](
              QObject *, QUrl, KiriView::ImageCandidatesCallback, KiriView::ErrorCallback) {
              ++directoryChangeSubscriptionCount;
              return KiriView::ImageIoJob();
          };

    KiriView::ImageNavigationCandidateProvider resolved
        = KiriView::imageNavigationCandidateProviderWithDefaults(std::move(provider));

    QVERIFY(resolved.directoryImages);
    QVERIFY(resolved.directoryContainers);
    QVERIFY(resolved.archiveImages);
    QVERIFY(resolved.directoryImageChanges);

    resolved.directoryImages(nullptr, QUrl(), {}, {});
    resolved.directoryImageChanges(nullptr, QUrl(), {}, {});
    QCOMPARE(directoryLoadCount, 1);
    QCOMPARE(directoryChangeSubscriptionCount, 1);
}

void TestRuntimeProviderDefaults::decodeDependencyDefaultsFillMissingFunctionsAndPreserveOverrides()
{
    int dataLoadCount = 0;
    KiriView::ImageDataLoader dataLoader
        = [&dataLoadCount](QObject *, KiriView::ImageDecodeRequest, KiriView::ImageDataCallback,
              KiriView::ErrorCallback) {
              ++dataLoadCount;
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDecodeDependencies resolved
        = KiriView::imageDecodeDependenciesWithDefaults({ std::move(dataLoader), {} });

    QVERIFY(resolved.dataLoader);
    QVERIFY(resolved.dataDecoder);

    resolved.dataLoader(nullptr, KiriView::ImageDecodeRequest(), {}, {});
    QCOMPARE(dataLoadCount, 1);
}

void TestRuntimeProviderDefaults::fileOperationDefaultFillsMissingProviderAndPreservesOverride()
{
    int fileOperationCount = 0;
    KiriView::FileOperationProvider fileOperations
        = [&fileOperationCount](
              QObject *, KiriView::FileDeletionRequest, KiriView::FileDeletionCallback) {
              ++fileOperationCount;
              return KiriView::ImageIoJob();
          };

    KiriView::FileOperationProvider resolved
        = KiriView::fileOperationProviderWithDefault(std::move(fileOperations));
    QVERIFY(resolved);
    resolved(nullptr, KiriView::FileDeletionRequest(), {});
    QCOMPARE(fileOperationCount, 1);

    QVERIFY(KiriView::fileOperationProviderWithDefault({}));
}

void TestRuntimeProviderDefaults::powerSaverDefaultFillsMissingProviderAndPreservesOverride()
{
    int monitorCount = 0;
    KiriView::PowerSaverProvider provider;
    provider.monitor = [&monitorCount](QObject *, KiriView::PowerSaverChangedCallback) {
        ++monitorCount;
        return std::unique_ptr<KiriView::PowerSaverStateMonitor>();
    };

    KiriView::PowerSaverProvider resolved
        = KiriView::powerSaverProviderWithDefault(std::move(provider));
    QVERIFY(resolved.monitor);
    resolved.monitor(nullptr, {});
    QCOMPARE(monitorCount, 1);

    QVERIFY(KiriView::powerSaverProviderWithDefault({}).monitor);
}

QTEST_GUILESS_MAIN(TestRuntimeProviderDefaults)
#include "test_runtimeproviderdefaults.moc"
