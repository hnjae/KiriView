// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagepredecodecoordinator.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <vector>

namespace {
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::decodeStaticTestImageData;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::testImage;

QUrl imageUrl(int index)
{
    return QUrl(QStringLiteral("file:///images/%1.png").arg(index, 2, 10, QLatin1Char('0')));
}

QUrl parentUrl() { return QUrl(QStringLiteral("file:///images/")); }

class FakeCandidateProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback) {
                if (callback) {
                    callback(directoryImagesByUrl[keyForUrl(directoryUrl)]);
                }
                return KiriView::ImageIoJob();
            },
            [](QObject *, QUrl, KiriView::ContainerCandidatesCallback callback,
                KiriView::ErrorCallback) {
                if (callback) {
                    callback({});
                }
                return KiriView::ImageIoJob();
            },
            [](QObject *, KiriView::ArchiveDocumentLocation,
                KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback) {
                if (callback) {
                    callback({});
                }
                return KiriView::ImageIoJob();
            },
        };
    }

    std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> directoryImagesByUrl;
};

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

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        testImage(),
    });

    const std::optional<KiriView::PredecodedImage> displayed = coordinator.tryTake(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, nextUrl);
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

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        testImage(),
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
