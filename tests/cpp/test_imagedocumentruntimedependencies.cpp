// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimedependencies.h"

#include "cache/imagecachepolicy.h"
#include "rendering/displayimagestore.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace {
kiriview::OpenedCollectionScopeLocation testArchiveCollection()
{
    return kiriview::OpenedCollectionScopeLocation::fromUrls(
        QUrl(QStringLiteral("file:///tmp/book.cbz")),
        QUrl(QStringLiteral("file:///tmp/book.cbz#/")),
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
}

class FakePowerSaverMonitor final : public kiriview::PowerSaverStateMonitor
{
public:
    bool powerSaverEnabled() const override { return true; }
};
}

class TestImageDocumentRuntimeDependencies : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultDependenciesUseMediaEntrySourceStore();
    void sharedDisplayStoreDefaultBudgetMatchesImageDocumentBudget();
    void partialNonSourceOverridesStillUseMediaEntrySourceStore();
    void customMediaEntrySourceFactoryWrapsOpenedCollectionProviders();
    void explicitOpenedCollectionProvidersAvoidMediaEntrySourceStore();
};

void TestImageDocumentRuntimeDependencies::defaultDependenciesUseMediaEntrySourceStore()
{
    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies({}, this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileDeletionProvider);
    QVERIFY(resolved.powerSaver.monitor);
    QVERIFY(resolved.cacheBudgets.predecodeCacheByteBudget > 0);
    QVERIFY(resolved.cacheBudgets.predecodeCacheByteBudget
        <= kiriview::predecodeCachePreferredByteBudget());
    QVERIFY(resolved.cacheBudgets.displayImageCacheByteBudget > 0);
    QVERIFY(resolved.cacheBudgets.displayImageCacheByteBudget
        <= kiriview::displayImageCachePreferredByteBudget());
}

void TestImageDocumentRuntimeDependencies::
    sharedDisplayStoreDefaultBudgetMatchesImageDocumentBudget()
{
    const kiriview::ImageCacheBudgets documentBudgets
        = kiriview::resolveImageDocumentCacheBudgets({});
    const std::shared_ptr<kiriview::DisplayImageStore> sharedStore
        = kiriview::sharedDisplayImageStore();

    QCOMPARE(sharedStore->byteBudget(), documentBudgets.displayImageCacheByteBudget);
}

void TestImageDocumentRuntimeDependencies::partialNonSourceOverridesStillUseMediaEntrySourceStore()
{
    int directoryLoadCount = 0;
    kiriview::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.directoryImageDocumentPages
        = [&directoryLoadCount](QObject *, QUrl,
              kiriview::ImageDocumentPageCandidatesCallback callback, kiriview::ErrorCallback) {
              ++directoryLoadCount;
              callback({});
              return kiriview::ImageIoJob();
          };

    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.imageDecode.dataLoader);

    bool candidatesReported = false;
    resolved.candidateProvider.directoryImageDocumentPages(nullptr,
        QUrl::fromLocalFile(QStringLiteral("/tmp/images/")),
        [&candidatesReported](
            std::vector<kiriview::ImageDocumentPageCandidate>) { candidatesReported = true; },
        {});
    QCOMPARE(directoryLoadCount, 1);
    QVERIFY(candidatesReported);
}

void TestImageDocumentRuntimeDependencies::
    customMediaEntrySourceFactoryWrapsOpenedCollectionProviders()
{
    int openCount = 0;
    kiriview::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.mediaEntrySourceFactory
        = [&openCount](const kiriview::OpenedCollectionScopeLocation &)
        -> kiriview::MediaEntrySourceOpenResult {
        ++openCount;
        return kiriview::MediaEntrySourceError { QStringLiteral("session failed") };
    };

    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);

    bool candidatesReported = false;
    QString errorString;
    resolved.candidateProvider.openedCollectionCandidates(
        nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<kiriview::ImageDocumentPageCandidate>) { candidatesReported = true; },
        [&errorString](const QString &error) { errorString = error; });

    QCOMPARE(openCount, 1);
    QVERIFY(!candidatesReported);
    QCOMPARE(errorString, QStringLiteral("session failed"));
}

void TestImageDocumentRuntimeDependencies::
    explicitOpenedCollectionProvidersAvoidMediaEntrySourceStore()
{
    int openedCollectionLoadCount = 0;
    int dataLoadCount = 0;
    int fileDeletionCount = 0;
    int powerSaverMonitorCount = 0;

    kiriview::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.openedCollectionCandidates
        = [&openedCollectionLoadCount](QObject *, kiriview::OpenedCollectionScopeLocation,
              kiriview::ImageDocumentPageCandidatesCallback callback, kiriview::ErrorCallback) {
              ++openedCollectionLoadCount;
              callback({});
              return kiriview::ImageIoJob();
          };
    dependencies.imageDecode.dataLoader
        = [&dataLoadCount](QObject *, kiriview::ImageDecodeRequest,
              kiriview::ImageDataCallback callback, kiriview::ErrorCallback) {
              ++dataLoadCount;
              callback(QByteArrayLiteral("custom image data"));
              return kiriview::ImageIoJob();
          };
    dependencies.fileDeletionProvider
        = [&fileDeletionCount](
              QObject *, kiriview::FileDeletionRequest, kiriview::FileDeletionCallback) {
              ++fileDeletionCount;
              return kiriview::ImageIoJob();
          };
    dependencies.powerSaver.monitor
        = [&powerSaverMonitorCount](QObject *, kiriview::PowerSaverChangedCallback) {
              ++powerSaverMonitorCount;
              return std::make_unique<FakePowerSaverMonitor>();
          };
    dependencies.cacheBudgetRequest.predecodeCacheByteBudget = 4096;
    dependencies.cacheBudgetRequest.displayImageCacheByteBudget = 8192;

    kiriview::ImageDocumentRuntimeDependencies resolved
        = kiriview::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(!resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileDeletionProvider);
    QVERIFY(resolved.powerSaver.monitor);
    QCOMPARE(resolved.cacheBudgets.predecodeCacheByteBudget, qsizetype(4096));
    QCOMPARE(resolved.cacheBudgets.displayImageCacheByteBudget, qsizetype(8192));

    bool candidatesReported = false;
    QByteArray loadedData;
    resolved.candidateProvider.openedCollectionCandidates(nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<kiriview::ImageDocumentPageCandidate>) { candidatesReported = true; },
        {});
    resolved.imageDecode.dataLoader(nullptr, kiriview::ImageDecodeRequest(),
        [&loadedData](QByteArray data) { loadedData = std::move(data); }, {});
    resolved.fileDeletionProvider(nullptr, kiriview::FileDeletionRequest(), {});
    std::unique_ptr<kiriview::PowerSaverStateMonitor> monitor
        = resolved.powerSaver.monitor(nullptr, {});

    QCOMPARE(openedCollectionLoadCount, 1);
    QVERIFY(candidatesReported);
    QCOMPARE(dataLoadCount, 1);
    QCOMPARE(loadedData, QByteArrayLiteral("custom image data"));
    QCOMPARE(fileDeletionCount, 1);
    QCOMPARE(powerSaverMonitorCount, 1);
    QVERIFY(monitor);
    QVERIFY(monitor->powerSaverEnabled());
}

QTEST_GUILESS_MAIN(TestImageDocumentRuntimeDependencies)
#include "test_imagedocumentruntimedependencies.moc"
