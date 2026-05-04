// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imageloader.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticDecodedTestImage;
using KiriView::TestSupport::testImage;

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

KiriView::DecodedImageResult decodeTestImageData(const QByteArray &data)
{
    if (data == QByteArrayLiteral("bad")) {
        return KiriView::DecodedImageFailure { QStringLiteral("decode failed") };
    }

    return staticDecodedTestImage();
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
            [this](QObject *, KiriView::ArchiveDocumentLocation archiveDocument,
                KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback) {
                loadImages(archiveImagesByUrl, archiveImageErrorsByUrl, archiveDocument.rootUrl(),
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
    return KiriView::ImageLoader(parent,
        KiriView::ImageAsyncDependencies {
            candidateProvider.provider(),
            dataLoaderFor(dataLoader),
            decodeTestImageData,
        });
}
}

class TestImageLoader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageLoadDeliversDecodedResult();
    void predecodedImageBypassesDataLoad();
    void comicBookArchiveResolvesFirstImage();
    void directArchiveResolvesFirstImage();
    void explicitKioArchiveImageStaysImageUrlMode();
    void archiveInteriorImageKeepsComicBookRoot();
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
    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl));
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
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    loader.setTakePredecodedImageCallback([imageUrl, archiveDocument](const QUrl &url) {
        if (url != imageUrl) {
            return std::optional<KiriView::PredecodedImage>();
        }

        const QImage image = testImage();
        return std::optional<KiriView::PredecodedImage>(KiriView::PredecodedImage {
            std::make_shared<KiriView::TestSupport::TestImageTileSource>(image), image,
            KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, *archiveDocument) });
    });

    std::optional<KiriView::ImageLoadSession> predecodedSession;
    QSize imageSize;
    loader.setPredecodedImageCallback(
        [&predecodedSession, &imageSize](
            KiriView::ImageLoadSession session, KiriView::PredecodedImage image) {
            predecodedSession = std::move(session);
            imageSize = image.preview.size();
        });

    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QVERIFY(predecodedSession.has_value());
    QCOMPARE(predecodedSession->location.imageUrl(), imageUrl);
    QCOMPARE(predecodedSession->location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
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

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(resolvedUrl, firstImageUrl);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, firstImageUrl);
}

void TestImageLoader::directArchiveResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.archiveImagesByUrl[keyForUrl(*archiveRootUrl)] = {
        imageCandidate(firstImageUrl),
    };

    QUrl resolvedUrl;
    std::optional<KiriView::ImageLoadSession> decodedSession;
    loader.setSourceResolvedCallback([&resolvedUrl](const QUrl &url) { resolvedUrl = url; });
    loader.setDecodedImageCallback([&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    });

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(resolvedUrl, firstImageUrl);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, firstImageUrl);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), firstImageUrl);
    QCOMPARE(decodedSession->location.archiveDocumentRootUrl(), *archiveRootUrl);
    QCOMPARE(
        decodedSession->location.archiveDocument().kind(), KiriView::ArchiveDocumentKind::General);
}

void TestImageLoader::explicitKioArchiveImageStaysImageUrlMode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    std::optional<KiriView::ImageLoadSession> decodedSession;
    loader.setDecodedImageCallback([&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    });

    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/02.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, imageUrl);
    QVERIFY(dataLoader.loads.front()->archiveDocument.isEmpty());
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), imageUrl);
    QVERIFY(decodedSession->location.archiveDocument().isEmpty());
}

void TestImageLoader::archiveInteriorImageKeepsComicBookRoot()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageLoader loader = createLoader(this, candidateProvider, dataLoader);

    std::optional<KiriView::ImageLoadSession> decodedSession;
    loader.setDecodedImageCallback([&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    });

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("02.png"));

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    loader.start(KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveDocument));

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, imageUrl);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), imageUrl);
    QCOMPARE(decodedSession->location.archiveDocumentRootUrl(), *archiveRootUrl);
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
    loader.start(KiriView::ImageLoadRequest::fromUrl(firstUrl));
    loader.start(KiriView::ImageLoadRequest::fromUrl(secondUrl));

    QCOMPARE(dataLoader.loads.size(), std::size_t(2));
    QVERIFY(dataLoader.loads.front()->canceled);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedUrls.size(), std::size_t(1));
    QCOMPARE(decodedUrls.front(), secondUrl);
}

QTEST_GUILESS_MAIN(TestImageLoader)

#include "test_imageloader.moc"
