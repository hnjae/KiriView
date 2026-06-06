// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/directorylistingjob.h"
#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "candidate_test_support.h"
#include "decoding/imagedecodedependencies.h"
#include "document/imagedocumentruntimedependencies.h"
#include "location/imagedocumentlocation.h"
#include "navigation/directmedianavigationcandidateprovider.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "system/filedeletion.h"
#include "system/powersaverprovider.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

struct ManualImageWorkerSchedule {
    KiriView::ImageWorkerOperation work;
    KiriView::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    KiriView::ImageWorkerScheduler scheduler()
    {
        return KiriView::ImageWorkerScheduler([this](QObject *, KiriView::ImageWorkerOperation work,
                                                  KiriView::ImageWorkerCompletion completion) {
            m_schedules.push_back(
                ManualImageWorkerSchedule { std::move(work), std::move(completion) });
        });
    }

    std::size_t scheduleCount() const { return m_schedules.size(); }

    void runWork(std::size_t index)
    {
        if (m_schedules.at(index).work) {
            m_schedules.at(index).work();
        }
    }

    void finish(std::size_t index)
    {
        if (m_schedules.at(index).completion) {
            m_schedules.at(index).completion();
        }
    }

private:
    std::vector<ManualImageWorkerSchedule> m_schedules;
};
}

class TestRuntimeProviderDefaults : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides();
    void candidateProviderDefaultsBindContainerLoaderToDirectoryProvider();
    void candidateProviderDefaultsBindOpenedCollectionLoaderToWorkerScheduler();
    void directMediaProviderDefaultBindsDirectoryProvider();
    void imageDocumentRuntimeDependenciesBindContainerLoaderToDirectoryProvider();
    void imageDocumentRuntimeDependenciesBindMediaEntryStoreToWorkerScheduler();
    void decodeDependencyDefaultsFillMissingFunctionsAndPreserveOverrides();
    void decodeDependencyDefaultsBindDataLoaderToWorkerScheduler();
    void decodeDependencyDefaultsBindThumbnailLookupToWorkerScheduler();
    void fileDeletionDefaultFillsMissingProviderAndPreservesOverride();
    void powerSaverDefaultFillsMissingProviderAndPreservesOverride();
};

void TestRuntimeProviderDefaults::candidateProviderDefaultsFillMissingLoadersAndPreserveOverrides()
{
    int directoryLoadCount = 0;
    int directoryChangeSubscriptionCount = 0;
    KiriView::ImageDocumentPageCandidateProvider provider;
    provider.directoryImageDocumentPages
        = [&directoryLoadCount](QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback,
              KiriView::ErrorCallback) {
              ++directoryLoadCount;
              return KiriView::ImageIoJob();
          };
    provider.directoryImageDocumentPageChanges
        = [&directoryChangeSubscriptionCount](QObject *, QUrl,
              KiriView::ImageDocumentPageCandidatesCallback, KiriView::ErrorCallback) {
              ++directoryChangeSubscriptionCount;
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentPageCandidateProvider resolved
        = KiriView::imageDocumentPageNavigationCandidateProviderWithDefaults(std::move(provider));

    QVERIFY(resolved.directoryImageDocumentPages);
    QVERIFY(resolved.directoryContainers);
    QVERIFY(resolved.openedCollectionCandidates);
    QVERIFY(resolved.directoryImageDocumentPageChanges);

    resolved.directoryImageDocumentPages(nullptr, QUrl(), {}, {});
    resolved.directoryImageDocumentPageChanges(nullptr, QUrl(), {}, {});
    QCOMPARE(directoryLoadCount, 1);
    QCOMPARE(directoryChangeSubscriptionCount, 1);
}

void TestRuntimeProviderDefaults::candidateProviderDefaultsBindContainerLoaderToDirectoryProvider()
{
    const QUrl requestedUrl = localUrl(QStringLiteral("/containers/"));
    QUrl providerUrl;
    int providerCallCount = 0;
    KiriView::DirectoryItemListProvider directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              KiriView::DirectoryItemListCallback callback, KiriView::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentPageCandidateProvider resolved
        = KiriView::imageDocumentPageNavigationCandidateProviderWithDefaults(
            KiriView::ImageDocumentPageCandidateProvider {}, {},
            std::move(directoryItemListProvider));
    QVERIFY(resolved.directoryContainers);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.directoryContainers(
        this, requestedUrl,
        [&callbackCount](std::vector<KiriView::ContainerNavigationCandidate>) { ++callbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(providerCallCount, 1);
    QCOMPARE(providerUrl, requestedUrl);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(errorCallbackCount, 0);
}

void TestRuntimeProviderDefaults::
    candidateProviderDefaultsBindOpenedCollectionLoaderToWorkerScheduler()
{
    ManualImageWorkerScheduler workerScheduler;
    KiriView::ImageDocumentPageCandidateProvider resolved
        = KiriView::imageDocumentPageNavigationCandidateProviderWithDefaults(
            KiriView::ImageDocumentPageCandidateProvider {}, workerScheduler.scheduler());
    QVERIFY(resolved.openedCollectionCandidates);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());

    int callbackCount = 0;
    int errorCallbackCount = 0;
    KiriView::ImageIoJob job = resolved.openedCollectionCandidates(
        this, *archiveCollection,
        [&callbackCount](std::vector<KiriView::ImageDocumentPageCandidate>) { ++callbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(callbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    workerScheduler.runWork(0);
    workerScheduler.finish(0);

    QVERIFY(!job.isActive());
    QCOMPARE(callbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
}

void TestRuntimeProviderDefaults::directMediaProviderDefaultBindsDirectoryProvider()
{
    const QUrl requestedUrl = localUrl(QStringLiteral("/media/"));
    QUrl providerUrl;
    int providerCallCount = 0;
    KiriView::DirectoryItemListProvider directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              KiriView::DirectoryItemListCallback callback, KiriView::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::DirectMediaNavigationCandidateProvider resolved
        = KiriView::directMediaNavigationCandidateProviderWithDefault(
            KiriView::DirectMediaNavigationCandidateProvider {},
            std::move(directoryItemListProvider));
    QVERIFY(resolved.directoryCandidateLoader);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.directoryCandidateLoader(
        this, requestedUrl,
        [&callbackCount](
            std::vector<KiriView::DirectMediaNavigationCandidate>) { ++callbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(providerCallCount, 1);
    QCOMPARE(providerUrl, requestedUrl);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(errorCallbackCount, 0);
}

void TestRuntimeProviderDefaults::
    imageDocumentRuntimeDependenciesBindContainerLoaderToDirectoryProvider()
{
    const QUrl requestedUrl = localUrl(QStringLiteral("/containers/"));
    QUrl providerUrl;
    int providerCallCount = 0;
    KiriView::ImageDocumentRuntimeDependencyOverrides overrides;
    overrides.directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              KiriView::DirectoryItemListCallback callback, KiriView::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(overrides), this);
    QVERIFY(resolved.candidateProvider.directoryContainers);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.candidateProvider.directoryContainers(
        this, requestedUrl,
        [&callbackCount](std::vector<KiriView::ContainerNavigationCandidate>) { ++callbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(providerCallCount, 1);
    QCOMPARE(providerUrl, requestedUrl);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(errorCallbackCount, 0);
}

void TestRuntimeProviderDefaults::
    imageDocumentRuntimeDependenciesBindMediaEntryStoreToWorkerScheduler()
{
    ManualImageWorkerScheduler workerScheduler;
    KiriView::ImageDocumentRuntimeDependencyOverrides overrides;
    overrides.imageDecode.workerScheduler = workerScheduler.scheduler();

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(overrides), this);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());

    int callbackCount = 0;
    int errorCallbackCount = 0;
    KiriView::ImageIoJob job = resolved.candidateProvider.openedCollectionCandidates(
        this, *archiveCollection,
        [&callbackCount](std::vector<KiriView::ImageDocumentPageCandidate>) { ++callbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(callbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    workerScheduler.runWork(0);
    workerScheduler.finish(0);

    QVERIFY(!job.isActive());
    QCOMPARE(callbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
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

void TestRuntimeProviderDefaults::decodeDependencyDefaultsBindDataLoaderToWorkerScheduler()
{
    ManualImageWorkerScheduler workerScheduler;
    KiriView::ImageDecodeDependencies dependencies;
    dependencies.workerScheduler = workerScheduler.scheduler();

    KiriView::ImageDecodeDependencies resolved
        = KiriView::imageDecodeDependenciesWithDefaults(std::move(dependencies));
    QVERIFY(resolved.dataLoader);

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    int dataCallbackCount = 0;
    int errorCallbackCount = 0;
    KiriView::ImageIoJob job = resolved.dataLoader(
        this,
        KiriView::ImageDecodeRequest::fromLocation(17,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                pageUrl, *archiveCollection)),
        [&dataCallbackCount](QByteArray) { ++dataCallbackCount; },
        [&errorCallbackCount](QString) { ++errorCallbackCount; });

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(dataCallbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    workerScheduler.runWork(0);
    workerScheduler.finish(0);

    QVERIFY(!job.isActive());
    QCOMPARE(dataCallbackCount, 0);
    QCOMPARE(errorCallbackCount, 0);
}

void TestRuntimeProviderDefaults::decodeDependencyDefaultsBindThumbnailLookupToWorkerScheduler()
{
    ManualImageWorkerScheduler workerScheduler;
    KiriView::ImageDecodeDependencies dependencies;
    dependencies.workerScheduler = workerScheduler.scheduler();

    KiriView::ImageDecodeDependencies resolved
        = KiriView::imageDecodeDependenciesWithDefaults(std::move(dependencies));
    QVERIFY(resolved.thumbnailPreviewLookupProvider);

    int callbackCount = 0;
    KiriView::ImageIoJob job
        = resolved.thumbnailPreviewLookupProvider(this, KiriView::ThumbnailCacheLookupRequest {},
            [&callbackCount](KiriView::ThumbnailCacheLookupResult) { ++callbackCount; });

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(callbackCount, 0);
    QVERIFY(job.isActive());

    job.cancel();
    workerScheduler.runWork(0);
    workerScheduler.finish(0);

    QVERIFY(!job.isActive());
    QCOMPARE(callbackCount, 0);
}

void TestRuntimeProviderDefaults::fileDeletionDefaultFillsMissingProviderAndPreservesOverride()
{
    int fileDeletionCount = 0;
    KiriView::FileDeletionProvider fileDeletionProvider
        = [&fileDeletionCount](
              QObject *, KiriView::FileDeletionRequest, KiriView::FileDeletionCallback) {
              ++fileDeletionCount;
              return KiriView::ImageIoJob();
          };

    KiriView::FileDeletionProvider resolved
        = KiriView::fileDeletionProviderWithDefault(std::move(fileDeletionProvider));
    QVERIFY(resolved);
    resolved(nullptr, KiriView::FileDeletionRequest(), {});
    QCOMPARE(fileDeletionCount, 1);

    QVERIFY(KiriView::fileDeletionProviderWithDefault({}));
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
