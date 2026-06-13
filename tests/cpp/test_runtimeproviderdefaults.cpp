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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;

struct ManualImageWorkerSchedule {
    kiriview::ImageWorkerOperation work;
    kiriview::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    kiriview::ImageWorkerScheduler scheduler()
    {
        return kiriview::ImageWorkerScheduler([this](QObject *, kiriview::ImageWorkerOperation work,
                                                  kiriview::ImageWorkerCompletion completion) {
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
    kiriview::ImageDocumentPageCandidateProvider provider;
    provider.directoryImageDocumentPages
        = [&directoryLoadCount](QObject *, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
              kiriview::ErrorCallback) {
              ++directoryLoadCount;
              return kiriview::ImageIoJob();
          };
    provider.directoryImageDocumentPageChanges
        = [&directoryChangeSubscriptionCount](QObject *, QUrl,
              kiriview::ImageDocumentPageCandidatesCallback, kiriview::ErrorCallback) {
              ++directoryChangeSubscriptionCount;
              return kiriview::ImageIoJob();
          };

    kiriview::ImageDocumentPageCandidateProvider resolved
        = kiriview::imageDocumentPageNavigationCandidateProviderWithDefaults(std::move(provider));

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
    kiriview::DirectoryItemListProvider directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              kiriview::DirectoryItemListCallback callback, kiriview::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return kiriview::ImageIoJob();
          };

    kiriview::ImageDocumentPageCandidateProvider resolved
        = kiriview::imageDocumentPageNavigationCandidateProviderWithDefaults(
            kiriview::ImageDocumentPageCandidateProvider {}, {},
            std::move(directoryItemListProvider));
    QVERIFY(resolved.directoryContainers);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.directoryContainers(
        this, requestedUrl,
        [&callbackCount](std::vector<kiriview::ContainerNavigationCandidate>) { ++callbackCount; },
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
    kiriview::ImageDocumentPageCandidateProvider resolved
        = kiriview::imageDocumentPageNavigationCandidateProviderWithDefaults(
            kiriview::ImageDocumentPageCandidateProvider {}, workerScheduler.scheduler());
    QVERIFY(resolved.openedCollectionCandidates);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());

    int callbackCount = 0;
    int errorCallbackCount = 0;
    kiriview::ImageIoJob job = resolved.openedCollectionCandidates(
        this, *archiveCollection,
        [&callbackCount](std::vector<kiriview::ImageDocumentPageCandidate>) { ++callbackCount; },
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
    kiriview::DirectoryItemListProvider directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              kiriview::DirectoryItemListCallback callback, kiriview::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return kiriview::ImageIoJob();
          };

    kiriview::DirectMediaNavigationCandidateProvider resolved
        = kiriview::directMediaNavigationCandidateProviderWithDefault(
            kiriview::DirectMediaNavigationCandidateProvider {},
            std::move(directoryItemListProvider));
    QVERIFY(resolved.directoryCandidateLoader);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.directoryCandidateLoader(
        this, requestedUrl,
        [&callbackCount](
            std::vector<kiriview::DirectMediaNavigationCandidate>) { ++callbackCount; },
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
    kiriview::ImageDocumentRuntimeDependencyOverrides overrides;
    overrides.directoryItemListProvider
        = [&providerCallCount, &providerUrl](QObject *, QUrl directoryUrl,
              kiriview::DirectoryItemListCallback callback, kiriview::ErrorCallback) {
              ++providerCallCount;
              providerUrl = std::move(directoryUrl);
              callback({});
              return kiriview::ImageIoJob();
          };

    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies(std::move(overrides), this);
    QVERIFY(resolved.candidateProvider.directoryContainers);

    int callbackCount = 0;
    int errorCallbackCount = 0;
    resolved.candidateProvider.directoryContainers(
        this, requestedUrl,
        [&callbackCount](std::vector<kiriview::ContainerNavigationCandidate>) { ++callbackCount; },
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
    kiriview::ImageDocumentRuntimeDependencyOverrides overrides;
    overrides.imageDecode.workerScheduler = workerScheduler.scheduler();

    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies(std::move(overrides), this);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());

    int callbackCount = 0;
    int errorCallbackCount = 0;
    kiriview::ImageIoJob job = resolved.candidateProvider.openedCollectionCandidates(
        this, *archiveCollection,
        [&callbackCount](std::vector<kiriview::ImageDocumentPageCandidate>) { ++callbackCount; },
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
    kiriview::ImageDataLoader dataLoader
        = [&dataLoadCount](QObject *, kiriview::ImageDecodeRequest, kiriview::ImageDataCallback,
              kiriview::ErrorCallback) {
              ++dataLoadCount;
              return kiriview::ImageIoJob();
          };

    kiriview::ImageDecodeDependencies resolved
        = kiriview::imageDecodeDependenciesWithDefaults({ std::move(dataLoader), {} });

    QVERIFY(resolved.dataLoader);
    QVERIFY(resolved.dataDecoder);

    resolved.dataLoader(nullptr, kiriview::ImageDecodeRequest(), {}, {});
    QCOMPARE(dataLoadCount, 1);
}

void TestRuntimeProviderDefaults::decodeDependencyDefaultsBindDataLoaderToWorkerScheduler()
{
    ManualImageWorkerScheduler workerScheduler;
    kiriview::ImageDecodeDependencies dependencies;
    dependencies.workerScheduler = workerScheduler.scheduler();

    kiriview::ImageDecodeDependencies resolved
        = kiriview::imageDecodeDependenciesWithDefaults(std::move(dependencies));
    QVERIFY(resolved.dataLoader);

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    int dataCallbackCount = 0;
    int errorCallbackCount = 0;
    kiriview::ImageIoJob job = resolved.dataLoader(
        this,
        kiriview::ImageDecodeRequest::fromLocation(17,
            kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
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
    kiriview::ImageDecodeDependencies dependencies;
    dependencies.workerScheduler = workerScheduler.scheduler();

    kiriview::ImageDecodeDependencies resolved
        = kiriview::imageDecodeDependenciesWithDefaults(std::move(dependencies));
    QVERIFY(resolved.thumbnailPreviewLookupProvider);

    int callbackCount = 0;
    kiriview::ImageIoJob job
        = resolved.thumbnailPreviewLookupProvider(this, kiriview::ThumbnailCacheLookupRequest {},
            [&callbackCount](kiriview::ThumbnailCacheLookupResult) { ++callbackCount; });

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
    kiriview::FileDeletionProvider fileDeletionProvider
        = [&fileDeletionCount](
              QObject *, kiriview::FileDeletionRequest, kiriview::FileDeletionCallback) {
              ++fileDeletionCount;
              return kiriview::ImageIoJob();
          };

    kiriview::FileDeletionProvider resolved
        = kiriview::fileDeletionProviderWithDefault(std::move(fileDeletionProvider));
    QVERIFY(resolved);
    resolved(nullptr, kiriview::FileDeletionRequest(), {});
    QCOMPARE(fileDeletionCount, 1);

    QVERIFY(kiriview::fileDeletionProviderWithDefault({}));
}

void TestRuntimeProviderDefaults::powerSaverDefaultFillsMissingProviderAndPreservesOverride()
{
    int monitorCount = 0;
    kiriview::PowerSaverProvider provider;
    provider.monitor = [&monitorCount](QObject *, kiriview::PowerSaverChangedCallback) {
        ++monitorCount;
        return std::unique_ptr<kiriview::PowerSaverStateMonitor>();
    };

    kiriview::PowerSaverProvider resolved
        = kiriview::powerSaverProviderWithDefault(std::move(provider));
    QVERIFY(resolved.monitor);
    resolved.monitor(nullptr, {});
    QCOMPARE(monitorCount, 1);

    QVERIFY(kiriview::powerSaverProviderWithDefault({}).monitor);
}

QTEST_GUILESS_MAIN(TestRuntimeProviderDefaults)
#include "test_runtimeproviderdefaults.moc"
