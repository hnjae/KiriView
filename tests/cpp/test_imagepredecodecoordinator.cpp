// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagepredecodecoordinator.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::decodeStaticTestImageData;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::testImage;
using KiriView::TestSupport::TestImageTileSource;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

QUrl imageUrl(int index)
{
    return QUrl(QStringLiteral("file:///images/%1.png").arg(index, 2, 10, QLatin1Char('0')));
}

QUrl parentUrl() { return QUrl(QStringLiteral("file:///images/")); }

KiriView::ImagePredecodeCoordinator createCoordinator(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return KiriView::ImagePredecodeCoordinator(parent,
        KiriView::ImageAsyncDependencies {
            candidateProvider.provider(),
            dataLoaderFor(dataLoader),
            decodeStaticTestImageData,
        });
}
}

class TestImagePredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImageAndPredecodesWindow();
    void cancelSuppressesPendingDecode();
};

void TestImagePredecodeCoordinator::scheduleCachesDisplayedImageAndPredecodesWindow()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl previousUrl = imageUrl(0);
    const QUrl displayedUrl = imageUrl(1);
    const QUrl nextUrl = imageUrl(2);
    candidateProvider.directoryImagesByUrl[keyForUrl(parentUrl())] = {
        imageCandidate(previousUrl),
        imageCandidate(displayedUrl),
        imageCandidate(nextUrl),
    };

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        std::make_shared<TestImageTileSource>(displayedImage),
        displayedImage,
        KiriView::StaticImageDisplayHints { 0.5 },
        KiriView::ImageFirstDisplayDecodeContext { QSize(640, 480) },
    });

    const std::optional<KiriView::PredecodedImage> displayed = coordinator.tryTake(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);
    QCOMPARE(displayed->displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, nextUrl);
    QCOMPARE(dataLoader.loads.front()->firstDisplay.physicalViewportSize, QSize(640, 480));
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.tryTake(nextUrl).has_value());
    QTRY_COMPARE(dataLoader.loads.size(), std::size_t(2));
    QCOMPARE(dataLoader.loads.back()->url, previousUrl);
}

void TestImagePredecodeCoordinator::cancelSuppressesPendingDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = imageUrl(1);
    const QUrl nextUrl = imageUrl(2);
    candidateProvider.directoryImagesByUrl[keyForUrl(parentUrl())] = {
        imageCandidate(displayedUrl),
        imageCandidate(nextUrl),
    };

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        std::make_shared<TestImageTileSource>(displayedImage),
        displayedImage,
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
