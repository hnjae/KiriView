// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace {
struct ManualLoad {
    QObject *object = nullptr;
    QUrl url;
    KiriView::ImageDecodeJob::DataCallback dataCallback;
    KiriView::ImageDecodeJob::ErrorCallback errorCallback;
    bool canceled = false;
};

class ManualImageDataLoader
{
public:
    KiriView::ImageIoJob start(QObject *receiver, QUrl url,
        KiriView::ImageDecodeJob::DataCallback callback,
        KiriView::ImageDecodeJob::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualLoad>();
        load->object = new QObject(receiver);
        load->url = std::move(url);
        load->dataCallback = std::move(callback);
        load->errorCallback = std::move(errorCallback);
        loads.push_back(load);

        return KiriView::ImageIoJob(load->object, [load](QObject *object) {
            load->canceled = true;
            if (object != nullptr) {
                object->deleteLater();
            }
        });
    }

    std::vector<std::shared_ptr<ManualLoad>> loads;
};

QString keyForUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments).toString(); }

QUrl imageUrl(int index)
{
    return QUrl(QStringLiteral("file:///images/%1.png").arg(index, 2, 10, QLatin1Char('0')));
}

QUrl parentUrl() { return QUrl(QStringLiteral("file:///images/")); }

KiriView::ImageNavigationCandidate imageCandidate(const QUrl &url)
{
    return KiriView::ImageNavigationCandidate { url, url.fileName() };
}

QImage testImage()
{
    QImage image(1, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

KiriView::DecodedImageResult decodeTestImageData(const QByteArray &)
{
    return KiriView::StaticDecodedImage { testImage() };
}

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
            [](QObject *, QUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback) {
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
    return KiriView::ImagePredecodeCoordinator(
        parent, candidateProvider.provider(),
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);
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
        KiriView::DisplayedImageLocation::fromUrls(displayedUrl),
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
        KiriView::DisplayedImageLocation::fromUrls(displayedUrl),
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
