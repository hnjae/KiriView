// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imageloader.h"

#include <QObject>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;
using KiriView::TestSupport::testImageDecodeFailureString;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::ImageLoader createLoader(QObject *parent, FakeCandidateProvider &candidateProvider,
    ManualImageDataLoader &dataLoader, KiriView::ImageLoader::Callbacks callbacks = {},
    KiriView::ImageDataDecoder dataDecoder = staticImageDataDecoderRejectingBadData())
{
    return KiriView::ImageLoader(parent, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, std::move(dataDecoder)), std::move(callbacks));
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
    void directDirectoryResolvesFirstImage();
    void explicitKioArchiveImageStaysImageUrlMode();
    void archiveInteriorImageKeepsComicBookRoot();
    void staleLoadResultIsIgnored();
};

void TestImageLoader::imageLoadDeliversDecodedResult()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageLoadSession> decodedSession;
    std::optional<KiriView::DecodedImage> decodedResult;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession, &decodedResult](KiriView::ImageLoadSession session,
                                 KiriView::DecodedImage result) {
        decodedSession = std::move(session);
        decodedResult = std::move(result);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl),
        KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) });
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(320, 240));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedResult.has_value());
    QVERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), imageUrl);
    QVERIFY(std::get_if<KiriView::StaticDecodedImage>(&*decodedResult) != nullptr);
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
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishFrontLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(errorSession.has_value());
    QCOMPARE(errorSession->location.imageUrl(), imageUrl);
    QCOMPARE(error, KiriView::ImageLoadError::Generic);
    QCOMPARE(errorString, testImageDecodeFailureString());
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
            staticTestImagePayload(image),
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
    QVERIFY(dataLoader.empty());
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
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
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
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), firstImageUrl);
    QCOMPARE(decodedSession->location.archiveDocumentRootUrl(), *archiveRootUrl);
    QCOMPARE(
        decodedSession->location.archiveDocument().kind(), KiriView::ArchiveDocumentKind::General);
}

void TestImageLoader::directDirectoryResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::ArchiveDocumentLocation> directoryDocument
        = KiriView::directOpenDocumentLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(directoryDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(directoryDocument->rootUrl(),
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

    loader.start(KiriView::ImageLoadRequest::fromUrl(directoryUrl));

    QCOMPARE(resolvedUrl, firstImageUrl);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location.imageUrl(), firstImageUrl);
    QCOMPARE(decodedSession->location.archiveDocumentRootUrl(), directoryDocument->rootUrl());
    QCOMPARE(decodedSession->location.archiveDocument().kind(),
        KiriView::ArchiveDocumentKind::Directory);
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

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    QVERIFY(dataLoader.frontLoad().archiveDocument.isEmpty());
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

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

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

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

    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("ok"));
    dataLoader.finishBackLoad(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedUrls.size(), std::size_t(1));
    QCOMPARE(decodedUrls.front(), secondUrl);
}

QTEST_GUILESS_MAIN(TestImageLoader)

#include "test_imageloader.moc"
