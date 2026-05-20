// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntimedependencies.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace {
KiriView::ArchiveDocumentLocation testArchiveDocument()
{
    return KiriView::ArchiveDocumentLocation::fromUrls(QUrl(QStringLiteral("file:///tmp/book.cbz")),
        QUrl(QStringLiteral("file:///tmp/book.cbz#/")), KiriView::ArchiveDocumentKind::ComicBook);
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
    void defaultDependenciesUseArchiveSessionStore();
    void partialNonArchiveOverridesStillUseArchiveSessionStore();
    void customSessionFactoryWrapsArchiveProviders();
    void explicitArchiveProvidersAvoidSessionStore();
};

void TestImageDocumentRuntimeDependencies::defaultDependenciesUseArchiveSessionStore()
{
    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies({}, this);

    QVERIFY(resolved.archiveSessionStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.archiveImages);
    QVERIFY(resolved.candidateProvider.directoryImageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);
}

void TestImageDocumentRuntimeDependencies::partialNonArchiveOverridesStillUseArchiveSessionStore()
{
    int directoryLoadCount = 0;
    KiriView::ImageAsyncDependencies dependencies;
    dependencies.candidateProvider.directoryImages
        = [&directoryLoadCount](QObject *, QUrl, KiriView::ImageCandidatesCallback callback,
              KiriView::ErrorCallback) {
              ++directoryLoadCount;
              callback({});
              return KiriView::ImageIoJob();
          };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.archiveSessionStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.archiveImages);
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
    KiriView::ImageAsyncDependencies dependencies;
    dependencies.archiveDocumentSessions = [&openCount](const KiriView::ArchiveDocumentLocation &)
        -> KiriView::ArchiveDocumentSessionOpenResult {
        ++openCount;
        return KiriView::ArchiveError { QStringLiteral("session failed") };
    };

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(resolved.archiveSessionStore);

    bool candidatesReported = false;
    QString errorString;
    resolved.candidateProvider.archiveImages(
        nullptr, testArchiveDocument(),
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

    KiriView::ImageAsyncDependencies dependencies;
    dependencies.candidateProvider.archiveImages
        = [&archiveLoadCount](QObject *, KiriView::ArchiveDocumentLocation,
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

    KiriView::ImageDocumentRuntimeDependencies resolved
        = KiriView::resolveImageDocumentRuntimeDependencies(std::move(dependencies), this);

    QVERIFY(!resolved.archiveSessionStore);
    QVERIFY(resolved.candidateProvider.directoryImages);
    QVERIFY(resolved.candidateProvider.directoryContainers);
    QVERIFY(resolved.candidateProvider.archiveImages);
    QVERIFY(resolved.candidateProvider.directoryImageChanges);
    QVERIFY(resolved.imageDecode.dataLoader);
    QVERIFY(resolved.imageDecode.dataDecoder);
    QVERIFY(resolved.fileOperations);
    QVERIFY(resolved.powerSaver.monitor);

    bool candidatesReported = false;
    QByteArray loadedData;
    resolved.candidateProvider.archiveImages(nullptr, testArchiveDocument(),
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
