// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimedependencies.h"

#include "cache/imagecachepolicy.h"
#include "rendering/staticimage.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace {
KiriView::OpenedCollectionScopeLocation testArchiveCollection()
{
    return KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl(QStringLiteral("file:///tmp/book.cbz")),
        QUrl(QStringLiteral("file:///tmp/book.cbz#/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}

class FakePowerSaverMonitor final : public KiriView::PowerSaverStateMonitor
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
    void partialNonSourceOverridesStillUseMediaEntrySourceStore();
    void customMediaEntrySourceFactoryWrapsOpenedCollectionProviders();
    void explicitOpenedCollectionProvidersAvoidMediaEntrySourceStore();
};

void TestImageDocumentRuntimeDependencies::defaultDependenciesUseMediaEntrySourceStore()
{
    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies({}, this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);
    QVERIFY(resolved.cacheBudgets.predecodeCacheByteBudget > 0);
    QVERIFY(resolved.cacheBudgets.predecodeCacheByteBudget
        <= KiriView::predecodeCachePreferredByteBudget());
    QVERIFY(resolved.cacheBudgets.staticTileCacheByteBudget > 0);
    QVERIFY(resolved.cacheBudgets.staticTileCacheByteBudget
        <= KiriView::imageFullDecodeFallbackByteLimit);
}

void TestImageDocumentRuntimeDependencies::partialNonSourceOverridesStillUseMediaEntrySourceStore()
{
    int directoryLoadCount = 0;
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.directoryImageDocumentPages
        = [&directoryLoadCount](QObject *, QUrl,
              KiriView::ImageDocumentPageCandidatesCallback callback, KiriView::ErrorCallback) {
              ++directoryLoadCount;
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.imageDecode.dataLoader);

    bool candidatesReported = false;
    resolved.candidateProvider.directoryImageDocumentPages(nullptr,
        QUrl::fromLocalFile(QStringLiteral("/tmp/images/")),
        [&candidatesReported](
            std::vector<KiriView::ImageDocumentPageCandidate>) { candidatesReported = true; },
        {});
    QCOMPARE(directoryLoadCount, 1);
    QVERIFY(candidatesReported);
}

void TestImageDocumentRuntimeDependencies::
    customMediaEntrySourceFactoryWrapsOpenedCollectionProviders()
{
    int openCount = 0;
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.mediaEntrySourceFactory
        = [&openCount](const KiriView::OpenedCollectionScopeLocation &)
        -> KiriView::MediaEntrySourceOpenResult {
        ++openCount;
        return KiriView::MediaEntrySourceError { QStringLiteral("session failed") };
    };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);

    bool candidatesReported = false;
    QString errorString;
    resolved.candidateProvider.openedCollectionCandidates(
        nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<KiriView::ImageDocumentPageCandidate>) { candidatesReported = true; },
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
    int fileOperationCount = 0;
    int powerSaverMonitorCount = 0;

    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.openedCollectionCandidates
        = [&openedCollectionLoadCount](QObject *, KiriView::OpenedCollectionScopeLocation,
              KiriView::ImageDocumentPageCandidatesCallback callback, KiriView::ErrorCallback) {
              ++openedCollectionLoadCount;
              callback({});
              return KiriView::ImageIoJob();
          };
    dependencies.imageDecode.dataLoader
        = [&dataLoadCount](QObject *, KiriView::ImageDecodeRequest,
              KiriView::ImageDataCallback callback, KiriView::ErrorCallback) {
              ++dataLoadCount;
              callback(QByteArrayLiteral("custom image data"));
              return KiriView::ImageIoJob();
          };
    dependencies.fileOperations = [&fileOperationCount](QObject *, KiriView::FileDeletionRequest,
                                      KiriView::FileDeletionCallback) {
        ++fileOperationCount;
        return KiriView::ImageIoJob();
    };
    dependencies.powerSaver.monitor
        = [&powerSaverMonitorCount](QObject *, KiriView::PowerSaverChangedCallback) {
              ++powerSaverMonitorCount;
              return std::make_unique<FakePowerSaverMonitor>();
          };
    dependencies.cacheBudgetRequest.predecodeCacheByteBudget = 4096;
    dependencies.cacheBudgetRequest.staticTileCacheByteBudget = 8192;

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(!resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageDocumentPageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);
    QCOMPARE(resolved.cacheBudgets.predecodeCacheByteBudget, qsizetype(4096));
    QCOMPARE(resolved.cacheBudgets.staticTileCacheByteBudget, qsizetype(8192));

    bool candidatesReported = false;
    QByteArray loadedData;
    resolved.candidateProvider.openedCollectionCandidates(nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<KiriView::ImageDocumentPageCandidate>) { candidatesReported = true; },
        {});
    resolved.imageDecode.dataLoader(nullptr, KiriView::ImageDecodeRequest(),
        [&loadedData](QByteArray data) { loadedData = std::move(data); }, {});
    resolved.fileOperations(nullptr, KiriView::FileDeletionRequest(), {});
    std::unique_ptr<KiriView::PowerSaverStateMonitor> monitor
        = resolved.powerSaver.monitor(nullptr, {});

    QCOMPARE(openedCollectionLoadCount, 1);
    QVERIFY(candidatesReported);
    QCOMPARE(dataLoadCount, 1);
    QCOMPARE(loadedData, QByteArrayLiteral("custom image data"));
    QCOMPARE(fileOperationCount, 1);
    QCOMPARE(powerSaverMonitorCount, 1);
    QVERIFY(monitor);
    QVERIFY(monitor->powerSaverEnabled());
}

QTEST_GUILESS_MAIN(TestImageDocumentRuntimeDependencies)
#include "test_imagedocumentruntimedependencies.moc"
