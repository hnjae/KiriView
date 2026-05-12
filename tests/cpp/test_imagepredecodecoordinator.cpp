// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDecodeDependenciesFor;
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
    return KiriView::ImagePredecodeCoordinator(parent, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()));
}
}

class TestImagePredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImageAndPredecodesWindow();
    void scheduleDisplayedImageUsesPresentationSnapshot();
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

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.tryTake(nextUrl).has_value());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
}

void TestImagePredecodeCoordinator::scheduleDisplayedImageUsesPresentationSnapshot()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);
    KiriView::ImagePresentationController presentation(this,
        []() {
            return KiriView::ImageDocumentRenderContext {
                2.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {});

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    presentation.setViewportSize(QSizeF(320.0, 240.0));
    presentation.setPredecodeCacheable(true);
    presentation.setStaticImage(
        staticTestImagePayload(testImage(QSize(10, 8)), KiriView::StaticImageDisplayHints { 0.5 }));

    coordinator.scheduleDisplayedImage(
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl), presentation);

    const std::optional<KiriView::PredecodedImage> displayed = coordinator.tryTake(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
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

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().archiveDocument.rootUrl(), archiveDocument->rootUrl());
    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));

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

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    coordinator.cancel();
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));
    QTest::qWait(50);
    QVERIFY(!coordinator.tryTake(nextUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
}

QTEST_GUILESS_MAIN(TestImagePredecodeCoordinator)

#include "test_imagepredecodecoordinator.moc"
