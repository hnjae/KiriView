// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imageloader.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <memory>
#include <optional>
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

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

KiriView::ImageNavigationCandidate imageCandidate(const QUrl &url)
{
    return KiriView::ImageNavigationCandidate { url, url.fileName() };
}

QImage testImage(int width = 1)
{
    QImage image(width, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

KiriView::DecodedImageResult decodeTestImageData(const QByteArray &data)
{
    if (data == QByteArrayLiteral("bad")) {
        return KiriView::DecodedImageFailure { QStringLiteral("decode failed") };
    }

    return KiriView::StaticDecodedImage { testImage() };
}

class FakeCandidateProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                loadImages(directoryImagesByUrl, directoryImageErrorsByUrl, std::move(directoryUrl),
                    std::move(callback), std::move(errorCallback));
                return KiriView::ImageIoJob();
            },
            [](QObject *, QUrl, KiriView::ContainerCandidatesCallback callback,
                KiriView::ErrorCallback) {
                if (callback) {
                    callback({});
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *, QUrl archiveRootUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                loadImages(archiveImagesByUrl, archiveImageErrorsByUrl, std::move(archiveRootUrl),
                    std::move(callback), std::move(errorCallback));
                return KiriView::ImageIoJob();
            },
        };
    }

    std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> directoryImagesByUrl;
    std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> archiveImagesByUrl;
    std::map<QString, QString> directoryImageErrorsByUrl;
    std::map<QString, QString> archiveImageErrorsByUrl;

private:
    void loadImages(std::map<QString, std::vector<KiriView::ImageNavigationCandidate>> &imagesByUrl,
        const std::map<QString, QString> &errorsByUrl, QUrl url,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        const QString key = keyForUrl(url);
        const auto error = errorsByUrl.find(key);
        if (error != errorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        if (callback) {
            callback(imagesByUrl[key]);
        }
    }
};

KiriView::ImageLoader createLoader(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return KiriView::ImageLoader(
        parent, candidateProvider.provider(),
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);
}
}

class TestImageLoader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageLoadDeliversDecodedResult();
    void predecodedImageBypassesDataLoad();
    void comicBookArchiveResolvesFirstImage();
    void staleLoadResultIsIgnored();
};

void TestImageLoader::imageLoadDeliversDecodedResult()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    std::optional<KiriView::ImageLoadSession> decodedSession;
    std::shared_ptr<KiriView::DecodedImageResult> decodedResult;
    loader.setDecodedImageCallback(
        [&decodedSession, &decodedResult](KiriView::ImageLoadSession session,
            std::shared_ptr<KiriView::DecodedImageResult> result) {
            decodedSession = std::move(session);
            decodedResult = std::move(result);
        });

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrls(imageUrl, QUrl()));
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, imageUrl);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedResult != nullptr);
    QVERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), imageUrl);
    QVERIFY(std::get_if<KiriView::StaticDecodedImage>(decodedResult.get()) != nullptr);
}

void TestImageLoader::predecodedImageBypassesDataLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    const QUrl imageUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl comicBookRootUrl = QUrl(QStringLiteral("zip:///books/book.cbz/"));
    loader.setTakePredecodedImageCallback([imageUrl, comicBookRootUrl](const QUrl &url) {
        if (url != imageUrl) {
            return std::optional<KiriView::PredecodedImage>();
        }

        return std::optional<KiriView::PredecodedImage>(KiriView::PredecodedImage {
            testImage(), KiriView::DisplayedImageLocation::fromUrls(imageUrl, comicBookRootUrl) });
    });

    std::optional<KiriView::ImageLoadSession> predecodedSession;
    QSize imageSize;
    loader.setPredecodedImageCallback(
        [&predecodedSession, &imageSize](KiriView::ImageLoadSession session, const QImage &image) {
            predecodedSession = std::move(session);
            imageSize = image.size();
        });

    loader.start(KiriView::ImageLoadRequest::fromUrls(imageUrl, QUrl()));

    QVERIFY(predecodedSession.has_value());
    QCOMPARE(predecodedSession->location.imageUrl(), imageUrl);
    QCOMPARE(predecodedSession->location.comicBookRootUrl(), comicBookRootUrl);
    QCOMPARE(imageSize, QSize(1, 1));
    QVERIFY(dataLoader.loads.empty());
}

void TestImageLoader::comicBookArchiveResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.archiveImagesByUrl[keyForUrl(*archiveRootUrl)] = {
        imageCandidate(firstImageUrl),
    };

    QUrl resolvedUrl;
    loader.setSourceResolvedCallback([&resolvedUrl](const QUrl &url) { resolvedUrl = url; });

    loader.start(KiriView::ImageLoadRequest::fromUrls(archiveUrl, QUrl()));

    QCOMPARE(resolvedUrl, firstImageUrl);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, firstImageUrl);
}

void TestImageLoader::staleLoadResultIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    std::vector<QUrl> decodedUrls;
    loader.setDecodedImageCallback([&decodedUrls](KiriView::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.location.imageUrl());
    });

    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrls(firstUrl, QUrl()));
    loader.start(KiriView::ImageLoadRequest::fromUrls(secondUrl, QUrl()));

    QCOMPARE(dataLoader.loads.size(), std::size_t(2));
    QVERIFY(dataLoader.loads.front()->canceled);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedUrls.size(), std::size_t(1));
    QCOMPARE(decodedUrls.front(), secondUrl);
}

QTEST_GUILESS_MAIN(TestImageLoader)

#include "test_imageloader.moc"
