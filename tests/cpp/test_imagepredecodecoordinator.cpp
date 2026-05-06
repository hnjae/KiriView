// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagepredecodecoordinator.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageAsyncDependenciesFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::ImagePredecodeCoordinator createCoordinator(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return KiriView::ImagePredecodeCoordinator(
        parent, imageAsyncDependenciesFor(candidateProvider, dataLoader, staticImageDataDecoder()));
}
}

class TestImagePredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImageAndPredecodesWindow();
    void archivePredecodeKeepsArchiveDocumentContext();
    void cancelSuppressesPendingDecode();
};

void TestImagePredecodeCoordinator::scheduleCachesDisplayedImageAndPredecodesWindow()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl previousUrl = indexedImageUrl(0);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageCandidate(previousUrl),
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        staticTestImagePayload(displayedImage, KiriView::StaticImageDisplayHints { 0.5 }),
        KiriView::ImageFirstDisplayDecodeContext { QSize(640, 480) },
    });

    const std::optional<KiriView::PredecodedImage> displayed = coordinator.tryTake(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);
    QCOMPARE(displayed->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, nextUrl);
    QCOMPARE(dataLoader.loads.front()->firstDisplay.physicalViewportSize, QSize(640, 480));
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.tryTake(nextUrl).has_value());
    QTRY_COMPARE(dataLoader.loads.size(), std::size_t(2));
    QCOMPARE(dataLoader.loads.back()->url, previousUrl);
}

void TestImagePredecodeCoordinator::archivePredecodeKeepsArchiveDocumentContext()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl displayedUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, *archiveDocument),
        false,
        staticTestImagePayload(testImage()),
    });

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, nextUrl);
    QCOMPARE(dataLoader.loads.front()->archiveDocument.rootUrl(), archiveDocument->rootUrl());
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.tryTake(nextUrl).has_value());
    const std::optional<KiriView::PredecodedImage> predecoded = coordinator.tryTake(nextUrl);
    QVERIFY(predecoded.has_value());
    QCOMPARE(predecoded->location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
}

void TestImagePredecodeCoordinator::cancelSuppressesPendingDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        staticTestImagePayload(displayedImage),
    });

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    coordinator.cancel();
    QVERIFY(dataLoader.loads.front()->canceled);

    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("next"));
    QTest::qWait(50);
    QVERIFY(!coordinator.tryTake(nextUrl).has_value());
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
}

QTEST_GUILESS_MAIN(TestImagePredecodeCoordinator)

#include "test_imagepredecodecoordinator.moc"
