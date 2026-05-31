// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageloader.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;
using KiriView::TestSupport::testImageDecodeFailureString;
using KiriView::TestSupport::videoCandidate;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageDocumentPageCandidateProvider;

struct ManualOpenedCollectionCandidateLoad {
    QObject *object = nullptr;
    KiriView::OpenedCollectionScopeLocation archiveCollection;
    KiriView::ImageDocumentPageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualOpenedCollectionCandidateProvider
{
public:
    KiriView::ImageIoJob start(QObject *receiver,
        KiriView::OpenedCollectionScopeLocation archiveCollection,
        KiriView::ImageDocumentPageCandidatesCallback callback,
        KiriView::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualOpenedCollectionCandidateLoad>();
        load->archiveCollection = std::move(archiveCollection);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        KiriView::ImageIoJob job = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
        m_loads.push_back(load);
        return job;
    }

    std::size_t loadCount() const { return m_loads.size(); }

    const ManualOpenedCollectionCandidateLoad &frontLoad() const { return *m_loads.front(); }

    void finishFrontLoad(std::vector<KiriView::ImageDocumentPageCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_loads.front(),
            [candidates = std::move(candidates)](
                ManualOpenedCollectionCandidateLoad &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    KiriView::ImageDocumentPageCandidateProvider provider()
    {
        return KiriView::ImageDocumentPageCandidateProvider {
            [](QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback,
                KiriView::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected directory image listing"));
                }
                return KiriView::ImageIoJob();
            },
            [](QObject *, QUrl, KiriView::ContainerCandidatesCallback,
                KiriView::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected container listing"));
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *receiver, KiriView::OpenedCollectionScopeLocation archiveCollection,
                KiriView::ImageDocumentPageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                return start(receiver, std::move(archiveCollection), std::move(callback),
                    std::move(errorCallback));
            },
            [](QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback,
                KiriView::ErrorCallback) { return KiriView::ImageIoJob(); },
        };
    }

private:
    std::vector<std::shared_ptr<ManualOpenedCollectionCandidateLoad>> m_loads;
};

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
    void mismatchedPredecodedImageFallsBackToDecode();
    void comicBookArchiveResolvesFirstImage();
    void directArchiveResolvesFirstVideoAsUnsupportedOpenedCollectionVideo();
    void directArchiveResolvesFirstImage();
    void directoryCollectionResolvesFirstImage();
    void explicitKioArchiveImageStaysImageUrlMode();
    void archiveInteriorVideoReportsUnsupportedOpenedCollectionVideo();
    void archiveInteriorImageKeepsComicBookRoot();
    void staleLoadResultIsIgnored();
    void staleArchiveListingResultIsIgnored();
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
    QCOMPARE(decodedSession->imageUrl(), imageUrl);
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
    QCOMPARE(errorSession->imageUrl(), imageUrl);
    QCOMPARE(error, KiriView::ImageLoadError::Generic);
    QCOMPARE(errorString, testImageDecodeFailureString());
    QCOMPARE(decodedCount, 0);
}

void TestImageLoader::predecodedImageBypassesDataLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    std::optional<KiriView::ImageLoadSession> predecodedSession;
    QSize imageSize;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.findPredecodedImage = [imageUrl, archiveCollection](const QUrl &url) {
        if (url != imageUrl) {
            return std::optional<KiriView::PredecodedImage>();
        }

        const QImage image = testImage();
        return std::optional<KiriView::PredecodedImage>(
            KiriView::PredecodedImage { staticTestImagePayload(image),
                KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                    imageUrl, *archiveCollection) });
    };
    callbacks.predecodedImage = [&predecodedSession, &imageSize](KiriView::ImageLoadSession session,
                                    KiriView::PredecodedImage image) {
        predecodedSession = std::move(session);
        imageSize = image.staticImage.preview.size();
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));

    QVERIFY(predecodedSession.has_value());
    QCOMPARE(predecodedSession->imageUrl(), imageUrl);
    QCOMPARE(
        predecodedSession->location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QCOMPARE(imageSize, QSize(1, 1));
    QVERIFY(dataLoader.empty());
}

void TestImageLoader::mismatchedPredecodedImageFallsBackToDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    int predecodedCount = 0;
    std::optional<KiriView::ImageLoadSession> decodedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.findPredecodedImage = [imageUrl, archiveCollection](const QUrl &url) {
        if (url != imageUrl) {
            return std::optional<KiriView::PredecodedImage>();
        }

        return std::optional<KiriView::PredecodedImage>(
            KiriView::PredecodedImage { staticTestImagePayload(testImage()),
                KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                    imageUrl, *archiveCollection) });
    };
    callbacks.predecodedImage
        = [&predecodedCount](KiriView::ImageLoadSession, auto) { ++predecodedCount; };
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(predecodedCount, 0);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    QVERIFY(dataLoader.frontLoad().openedCollectionScope.isEmpty());

    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));
    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), imageUrl);
    QVERIFY(decodedSession->openedCollectionScope().isEmpty());
}

void TestImageLoader::comicBookArchiveResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<KiriView::ImageLoadSession> resolvedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](KiriView::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl),
        KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) });

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), firstImageUrl);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(320, 240));
}

void TestImageLoader::directArchiveResolvesFirstVideoAsUnsupportedOpenedCollectionVideo()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstVideoUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.mp4"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            videoCandidate(firstVideoUrl),
        });

    std::optional<KiriView::ImageLoadSession> resolvedSession;
    std::optional<KiriView::ImageLoadSession> unsupportedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](KiriView::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.unsupportedOpenedCollectionVideo
        = [&unsupportedSession](
              KiriView::ImageLoadSession session) { unsupportedSession = std::move(session); };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), firstVideoUrl);
    QVERIFY(unsupportedSession.has_value());
    QCOMPARE(unsupportedSession->imageUrl(), firstVideoUrl);
    QCOMPARE(unsupportedSession->openedCollectionScope().rootUrl(), *archiveRootUrl);
    QVERIFY(dataLoader.empty());
}

void TestImageLoader::directArchiveResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<QUrl> archiveRootUrl = KiriView::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<KiriView::ImageLoadSession> decodedSession;
    std::optional<KiriView::ImageLoadSession> resolvedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](KiriView::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), firstImageUrl);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), firstImageUrl);
    QCOMPARE(decodedSession->location().openedCollectionScopeRootUrl(), *archiveRootUrl);
    QCOMPARE(decodedSession->openedCollectionScope().kind(),
        KiriView::OpenedCollectionScopeKind::GeneralArchive);
}

void TestImageLoader::directoryCollectionResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(directoryCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<KiriView::ImageLoadSession> decodedSession;
    std::optional<KiriView::ImageLoadSession> resolvedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](KiriView::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.decodedImage = [&decodedSession](KiriView::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(KiriView::ImageLoadRequest::fromUrl(directoryUrl));

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), firstImageUrl);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), firstImageUrl);
    QCOMPARE(
        decodedSession->location().openedCollectionScopeRootUrl(), directoryCollection->rootUrl());
    QCOMPARE(decodedSession->openedCollectionScope().kind(),
        KiriView::OpenedCollectionScopeKind::Directory);
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
    QVERIFY(dataLoader.frontLoad().openedCollectionScope.isEmpty());
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), imageUrl);
    QVERIFY(decodedSession->openedCollectionScope().isEmpty());
}

void TestImageLoader::archiveInteriorVideoReportsUnsupportedOpenedCollectionVideo()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageLoadSession> unsupportedSession;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.unsupportedOpenedCollectionVideo
        = [&unsupportedSession](
              KiriView::ImageLoadSession session) { unsupportedSession = std::move(session); };
    KiriView::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl videoUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.bin"));

    loader.start(KiriView::ImageLoadRequest::fromTarget(
        KiriView::ImageDocumentPageTarget {
            videoUrl,
            KiriView::ImageDocumentPageKind::Video,
        },
        *archiveCollection));

    QVERIFY(unsupportedSession.has_value());
    QCOMPARE(unsupportedSession->imageUrl(), videoUrl);
    QCOMPARE(unsupportedSession->kind(), KiriView::ImageDocumentPageKind::Video);
    QCOMPARE(unsupportedSession->location().openedCollectionScopeRootUrl(),
        archiveCollection->rootUrl());
    QVERIFY(dataLoader.empty());
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

    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    loader.start(KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location().imageUrl(), imageUrl);
    QCOMPARE(decodedSession->location().openedCollectionScopeRootUrl(), *archiveRootUrl);
}

void TestImageLoader::staleLoadResultIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<QUrl> decodedUrls;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedUrls](KiriView::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.imageUrl());
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

void TestImageLoader::staleArchiveListingResultIsIgnored()
{
    ManualOpenedCollectionCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<QUrl> decodedUrls;
    KiriView::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&decodedUrls](KiriView::ImageLoadSession session) {
        decodedUrls.push_back(session.imageUrl());
    };
    callbacks.decodedImage = [&decodedUrls](KiriView::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.imageUrl());
    };
    KiriView::ImageLoader loader(this, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl staleImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/replacement.png"));

    loader.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl));
    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));

    loader.start(KiriView::ImageLoadRequest::fromUrl(replacementUrl));
    QVERIFY(candidateProvider.frontLoad().canceled);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, replacementUrl);

    candidateProvider.finishFrontLoad({ imageDocumentPageCandidate(staleImageUrl) });
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));

    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedUrls.size(), std::size_t(1));
    QCOMPARE(decodedUrls.front(), replacementUrl);
}

QTEST_GUILESS_MAIN(TestImageLoader)

#include "test_imageloader.moc"
