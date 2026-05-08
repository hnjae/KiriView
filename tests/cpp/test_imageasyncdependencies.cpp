// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncdependencies.h"

#include "imageiojob.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>

class TestImageAsyncDependencies : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides();
    void decodeDependencyDefaultsFillMissingFunctionsAndPreserveOverrides();
    void asyncDependencyDefaultsFillAggregateAndPreserveFileOperations();
};

void TestImageAsyncDependencies::candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides()
{
    int directoryLoadCount = 0;
    KiriView::ImageNavigationCandidateProvider provider;
    provider.directoryImages = [&directoryLoadCount](QObject *, QUrl,
                                   KiriView::ImageCandidatesCallback, KiriView::ErrorCallback) {
        ++directoryLoadCount;
        return KiriView::ImageIoJob();
    };

    KiriView::ImageNavigationCandidateProvider resolved
        = KiriView::imageNavigationCandidateProviderWithDefaults(std::move(provider));

    QVERIFY(resolved.directoryImages);
    QVERIFY(resolved.directoryContainers);
    QVERIFY(resolved.archiveImages);

    resolved.directoryImages(nullptr, QUrl(), {}, {});
    QCOMPARE(directoryLoadCount, 1);
}

void TestImageAsyncDependencies::decodeDependencyDefaultsFillMissingFunctionsAndPreserveOverrides()
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

void TestImageAsyncDependencies::asyncDependencyDefaultsFillAggregateAndPreserveFileOperations()
{
    int fileOperationCount = 0;
    KiriView::FileOperationProvider fileOperations
        = [&fileOperationCount](
              QObject *, KiriView::FileDeletionRequest, KiriView::FileDeletionCallback) {
              ++fileOperationCount;
              return KiriView::ImageIoJob();
          };

    KiriView::ImageAsyncDependencies dependencies;
    dependencies.fileOperations = std::move(fileOperations);

    KiriView::ImageAsyncDependencies resolved
        = KiriView::imageAsyncDependenciesWithDefaults(std::move(dependencies));

    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.archiveImages);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);

    resolved.fileOperations(nullptr, KiriView::FileDeletionRequest(), {});
    QCOMPARE(fileOperationCount, 1);
}

QTEST_GUILESS_MAIN(TestImageAsyncDependencies)
#include "test_imageasyncdependencies.moc"
