// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimedependencies.h"

#include "cache/imagecachepolicy.h"

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
    void customSessionFactoryWrapsArchiveProviders();
    void explicitArchiveProvidersAvoidSessionStore();
};

void TestImageDocumentRuntimeDependencies::defaultDependenciesUseMediaEntrySourceStore()
{
    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies({}, this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);
    QVERIFY(resolved.predecodeCacheByteBudget > 0);
    QVERIFY(resolved.predecodeCacheByteBudget <= KiriView::predecodeCachePreferredByteBudget());
}

void TestImageDocumentRuntimeDependencies::partialNonSourceOverridesStillUseMediaEntrySourceStore()
{
    int directoryLoadCount = 0;
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.directoryImages
        = [&directoryLoadCount](QObject *, QUrl, KiriView::ImageCandidatesCallback callback,
              KiriView::ErrorCallback) {
              ++directoryLoadCount;
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.imageDecode.dataLoader);

    bool candidatesReported = false;
    resolved.candidateProvider.directoryImages(nullptr,
        QUrl::fromLocalFile(QStringLiteral("/tmp/images/")),
        [&candidatesReported](
            std::vector<KiriView::ImageNavigationCandidate>) { candidatesReported = true; },
        {});
    QCOMPARE(directoryLoadCount, 1);
    QVERIFY(candidatesReported);
}

void TestImageDocumentRuntimeDependencies::customSessionFactoryWrapsArchiveProviders()
{
    int openCount = 0;
    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.mediaEntrySourceFactory
        = [&openCount](const KiriView::OpenedCollectionScopeLocation &)
        -> KiriView::MediaEntrySourceOpenResult {
        ++openCount;
        return KiriView::ArchiveError { QStringLiteral("session failed") };
    };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.mediaEntrySourceStore);

    bool candidatesReported = false;
    QString errorString;
    resolved.candidateProvider.openedCollectionCandidates(
        nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<KiriView::ImageNavigationCandidate>) { candidatesReported = true; },
        [&errorString](const QString &error) { errorString = error; });

    QCOMPARE(openCount, 1);
    QVERIFY(!candidatesReported);
    QCOMPARE(errorString, QStringLiteral("session failed"));
}

void TestImageDocumentRuntimeDependencies::explicitArchiveProvidersAvoidSessionStore()
{
    int archiveLoadCount = 0;
    int dataLoadCount = 0;
    int fileOperationCount = 0;
    int powerSaverMonitorCount = 0;

    KiriView::ImageDocumentRuntimeDependencyOverrides dependencies;
    dependencies.candidateProvider.openedCollectionCandidates
        = [&archiveLoadCount](QObject *, KiriView::OpenedCollectionScopeLocation,
              KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback) {
              ++archiveLoadCount;
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
    dependencies.predecodeCacheByteBudget = 4096;

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(!resolved.mediaEntrySourceStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.openedCollectionCandidates);
    QVERIFY(resolved.candidateProvider.directoryImageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);
    QCOMPARE(resolved.predecodeCacheByteBudget, qsizetype(4096));

    bool candidatesReported = false;
    QByteArray loadedData;
    resolved.candidateProvider.openedCollectionCandidates(nullptr, testArchiveCollection(),
        [&candidatesReported](
            std::vector<KiriView::ImageNavigationCandidate>) { candidatesReported = true; },
        {});
    resolved.imageDecode.dataLoader(nullptr, KiriView::ImageDecodeRequest(),
        [&loadedData](QByteArray data) { loadedData = std::move(data); }, {});
    resolved.fileOperations(nullptr, KiriView::FileDeletionRequest(), {});
    std::unique_ptr<KiriView::PowerSaverStateMonitor> monitor
        = resolved.powerSaver.monitor(nullptr, {});

    QCOMPARE(archiveLoadCount, 1);
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
