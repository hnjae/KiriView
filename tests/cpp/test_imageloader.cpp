// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imageloader.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticDecodedTestImage;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::DecodedImageResult decodeTestImageData(
    const QByteArray &data, const KiriView::ImageDecodeRequest &)
{
    if (data == QByteArrayLiteral("bad")) {
        return KiriView::DecodedImageFailure { QStringLiteral("decode failed") };
    }

    return staticDecodedTestImage();
}

KiriView::ImageLoader createLoader(QObject *parent, FakeCandidateProvider &candidateProvider,
    ManualImageDataLoader &dataLoader, KiriView::ImageLoader::Callbacks callbacks = {})
{
    return KiriView::ImageLoader(parent,
        KiriView::ImageAsyncDependencies {
            candidateProvider.provider(),
            dataLoaderFor(dataLoader),
            decodeTestImageData,
        },
        std::move(callbacks));
}
}

class TestImageLoader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageLoadDeliversDecodedResult();
    void decodeFailureUsesErrorCallback();
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
    std::optional<KiriView::ImageLoadSession> decodedSession;
    std::shared_ptr<KiriView::DecodedImage> decodedResult;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession, &decodedResult](KiriView::ImageLoadSession session,
                                 std::shared_ptr<KiriView::DecodedImage> result) {
        decodedSession = std::move(session);
        decodedResult = std::move(result);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl),
        KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) });
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, imageUrl);
    QCOMPARE(dataLoader.loads.front()->firstDisplay.physicalViewportSize, QSize(320, 240));
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedResult != nullptr);
    QVERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), imageUrl);
    QVERIFY(std::get_if<KiriView::StaticDecodedImage>(decodedResult.get()) != nullptr);
}

void TestImageLoader::decodeFailureUsesErrorCallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageLoadSession> errorSession;
    KiriView::ImageLoadError error = KiriView::ImageLoadError::Generic;
    QString errorString;
    int decodedCount = 0;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &error, &errorString](KiriView::ImageLoadSession session,
                          KiriView::ImageLoadError loadError, const QString &message) {
        errorSession = std::move(session);
        error = loadError;
        errorString = message;
    };
    callbacks.decodedImage = [&decodedCount](KiriView::ImageLoadSession, auto) { ++decodedCount; };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/bad.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl));
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("bad"));

    QTRY_VERIFY(errorSession.has_value());
    QCOMPARE(errorSession->location.imageUrl(), imageUrl);
    QCOMPARE(error, KiriView::ImageLoadError::Generic);
    QCOMPARE(errorString, QStringLiteral("decode failed"));
    QCOMPARE(decodedCount, 0);
}

void TestImageLoader::predecodedImageBypassesDataLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    std::optional<KiriView::ImageLoadSession> predecodedSession;
    QSize imageSize;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.takePredecodedImage = [imageUrl, archiveDocument](const QUrl &url) {
        if (url != imageUrl) {
            return std::optional<KiriView::PredecodedImage>();
        }

        const QImage image = testImage();
        return std::optional<KiriView::PredecodedImage>(KiriView::PredecodedImage {
            KiriView::StaticImagePayload {
                std::make_shared<KiriView::TestSupport::TestImageTileSource>(image), image, {} },
            KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, *archiveDocument) });
    };
    callbacks.predecodedImage = [&predecodedSession, &imageSize](KiriView::ImageLoadSession session,
                                    KiriView::PredecodedImage image) {
        predecodedSession = std::move(session);
        imageSize = image.staticImage.preview.size();
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

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

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(*archiveRootUrl,
        {
            imageCandidate(firstImageUrl),
        });

    QUrl resolvedUrl;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedUrl](const QUrl &url) { resolvedUrl = url; };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(resolvedUrl, firstImageUrl);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->url, firstImageUrl);
}

void TestImageLoader::directArchiveResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(*archiveRootUrl,
        {
            imageCandidate(firstImageUrl),
        });

    QUrl resolvedUrl;
    std::optional<KiriView::ImageLoadSession> decodedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedUrl](const QUrl &url) { resolvedUrl = url; };
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

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
    std::optional<KiriView::ImageLoadSession> decodedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

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
    std::optional<KiriView::ImageLoadSession> decodedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

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
    std::vector<QUrl> decodedUrls;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedUrls](KiriView::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.location.imageUrl());
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

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
