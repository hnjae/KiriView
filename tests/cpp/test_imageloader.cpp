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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoderRejectingBadData;
using kiriview::TestSupport::testImage;
using kiriview::TestSupport::testImageDecodeFailureString;
using kiriview::TestSupport::videoCandidate;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

struct ManualOpenedCollectionCandidateLoad
{
    QObject* object = nullptr;
    kiriview::OpenedCollectionScopeLocation archiveCollection;
    kiriview::ImageDocumentPageCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualOpenedCollectionCandidateProvider
{
public:
    kiriview::ImageIoJob start(QObject* receiver,
        kiriview::OpenedCollectionScopeLocation archiveCollection,
        kiriview::ImageDocumentPageCandidatesCallback callback,
        kiriview::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualOpenedCollectionCandidateLoad>();
        load->archiveCollection = std::move(archiveCollection);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        kiriview::ImageIoJob job = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
        m_loads.push_back(load);
        return job;
    }

    std::size_t loadCount() const { return m_loads.size(); }

    const ManualOpenedCollectionCandidateLoad& frontLoad() const { return *m_loads.front(); }

    void finishFrontLoad(std::vector<kiriview::ImageDocumentPageCandidate> candidates)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(m_loads.front(),
            [candidates = std::move(candidates)](
                ManualOpenedCollectionCandidateLoad& load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void failFrontLoad(const QString& errorString)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(
            m_loads.front(), [&errorString](ManualOpenedCollectionCandidateLoad& load) {
                if (load.errorCallback) {
                    load.errorCallback(errorString);
                }
            });
    }

    kiriview::ImageDocumentPageCandidateProvider provider()
    {
        return kiriview::ImageDocumentPageCandidateProvider {
            [](QObject*, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
                kiriview::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected directory image listing"));
                }
                return kiriview::ImageIoJob();
            },
            [](QObject*, QUrl, kiriview::ContainerCandidatesCallback,
                kiriview::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected container listing"));
                }
                return kiriview::ImageIoJob();
            },
            [this](QObject* receiver, kiriview::OpenedCollectionScopeLocation archiveCollection,
                kiriview::ImageDocumentPageCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                return start(receiver, std::move(archiveCollection), std::move(callback),
                    std::move(errorCallback));
            },
            [](QObject*, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
                kiriview::ErrorCallback) { return kiriview::ImageIoJob(); },
        };
    }

private:
    std::vector<std::shared_ptr<ManualOpenedCollectionCandidateLoad>> m_loads;
};

kiriview::ImageLoader createLoader(QObject* parent, FakeCandidateProvider& candidateProvider,
    ManualImageDataLoader& dataLoader, kiriview::ImageLoader::Callbacks callbacks = {},
    kiriview::ImageDataDecoder dataDecoder = staticImageDataDecoderRejectingBadData())
{
    return kiriview::ImageLoader(parent, candidateProvider.provider(),
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
    void directoryCollectionVideoReportsUnsupportedOpenedCollectionVideo();
    void explicitKioArchiveImageStaysImageUrlMode();
    void archiveInteriorVideoReportsUnsupportedOpenedCollectionVideo();
    void archiveInteriorImageKeepsComicBookRoot();
    void loadErrorPreservesTypedFailureMetadata();
    void decodeFailurePreservesTypedFailureMetadata();
    void openedCollectionCandidateFailurePreservesTypedFailureMetadata();
    void emptyOpenedCollectionPreservesTypedFailureMetadata();
    void staleLoadResultIsIgnored();
    void staleArchiveListingResultIsIgnored();
};

void TestImageLoader::imageLoadDeliversDecodedResult()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> decodedSession;
    std::optional<kiriview::DecodedImage> decodedResult;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession, &decodedResult](kiriview::ImageLoadSession session,
                                 kiriview::DecodedImage result) {
        decodedSession = std::move(session);
        decodedResult = std::move(result);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl),
        kiriview::ImageFirstDisplayDecodeContext { QSize(320, 240) });
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(320, 240));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedResult.has_value());
    QVERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), imageUrl);
    QVERIFY(std::get_if<kiriview::StaticDecodedImage>(&*decodedResult) != nullptr);
}

void TestImageLoader::decodeFailureUsesErrorCallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> errorSession;
    std::optional<kiriview::ImageLoadFailure> failure;
    int decodedCount = 0;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &failure](kiriview::ImageLoadSession session,
                          kiriview::ImageLoadFailure loadFailure) {
        errorSession = std::move(session);
        failure = std::move(loadFailure);
    };
    callbacks.decodedImage = [&decodedCount](kiriview::ImageLoadSession, auto) { ++decodedCount; };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/bad.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl));
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishFrontLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(errorSession.has_value());
    QVERIFY(failure.has_value());
    QCOMPARE(errorSession->imageUrl(), imageUrl);
    QVERIFY(failure->kind == kiriview::ImageLoadFailureKind::Decode);
    QCOMPARE(failure->userMessage, testImageDecodeFailureString());
    QCOMPARE(decodedCount, 0);
}

void TestImageLoader::predecodedImageBypassesDataLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    std::optional<kiriview::ImageLoadSession> predecodedSession;
    QSize imageSize;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.findPredecodedImage = [imageUrl, archiveCollection](const QUrl& url) {
        if (url != imageUrl) {
            return std::optional<kiriview::PredecodedImage>();
        }

        const QImage image = testImage();
        return std::optional<kiriview::PredecodedImage>(
            kiriview::PredecodedImage { staticDisplayTestImagePayload(image),
                kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
                    imageUrl, *archiveCollection) });
    };
    callbacks.predecodedImage = [&predecodedSession, &imageSize](kiriview::ImageLoadSession session,
                                    kiriview::PredecodedImage image) {
        predecodedSession = std::move(session);
        imageSize = image.displayImage.image.size();
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));

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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    int predecodedCount = 0;
    std::optional<kiriview::ImageLoadSession> decodedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.findPredecodedImage = [imageUrl, archiveCollection](const QUrl& url) {
        if (url != imageUrl) {
            return std::optional<kiriview::PredecodedImage>();
        }

        return std::optional<kiriview::PredecodedImage>(
            kiriview::PredecodedImage { staticDisplayTestImagePayload(testImage()),
                kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
                    imageUrl, *archiveCollection) });
    };
    callbacks.predecodedImage
        = [&predecodedCount](kiriview::ImageLoadSession, auto) { ++predecodedCount; };
    callbacks.decodedImage = [&decodedSession](kiriview::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl));

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
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<kiriview::ImageLoadSession> resolvedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](kiriview::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl),
        kiriview::ImageFirstDisplayDecodeContext { QSize(320, 240) });

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
    const std::optional<QUrl> archiveRootUrl = kiriview::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstVideoUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.mp4"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            videoCandidate(firstVideoUrl),
        });

    std::optional<kiriview::ImageLoadSession> resolvedSession;
    std::optional<kiriview::ImageLoadSession> unsupportedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](kiriview::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.unsupportedOpenedCollectionVideo
        = [&unsupportedSession](
              kiriview::ImageLoadSession session) { unsupportedSession = std::move(session); };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl));

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
    const std::optional<QUrl> archiveRootUrl = kiriview::directArchiveOpenRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl,
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<kiriview::ImageLoadSession> decodedSession;
    std::optional<kiriview::ImageLoadSession> resolvedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](kiriview::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.decodedImage = [&decodedSession](kiriview::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl));

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), firstImageUrl);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, firstImageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->imageUrl(), firstImageUrl);
    QCOMPARE(decodedSession->location().openedCollectionScopeRootUrl(), *archiveRootUrl);
    QCOMPARE(decodedSession->openedCollectionScope().kind(),
        kiriview::OpenedCollectionScopeKind::GeneralArchive);
}

void TestImageLoader::directoryCollectionResolvesFirstImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(directoryCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
        });

    std::optional<kiriview::ImageLoadSession> decodedSession;
    std::optional<kiriview::ImageLoadSession> resolvedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](kiriview::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.decodedImage = [&decodedSession](kiriview::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(directoryUrl));

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
        kiriview::OpenedCollectionScopeKind::Directory);
}

void TestImageLoader::directoryCollectionVideoReportsUnsupportedOpenedCollectionVideo()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    const QUrl videoUrl = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("01.mp4"));
    candidateProvider.setOpenedCollectionCandidates(directoryCollection->rootUrl(),
        {
            videoCandidate(videoUrl),
        });

    std::optional<kiriview::ImageLoadSession> resolvedSession;
    std::optional<kiriview::ImageLoadSession> unsupportedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&resolvedSession](kiriview::ImageLoadSession session) {
        resolvedSession = std::move(session);
    };
    callbacks.unsupportedOpenedCollectionVideo
        = [&unsupportedSession](
              kiriview::ImageLoadSession session) { unsupportedSession = std::move(session); };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    loader.start(kiriview::ImageLoadRequest::fromUrl(directoryUrl));

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->imageUrl(), videoUrl);
    QVERIFY(unsupportedSession.has_value());
    QCOMPARE(unsupportedSession->imageUrl(), videoUrl);
    QCOMPARE(unsupportedSession->kind(), kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(unsupportedSession->openedCollectionScope().kind(),
        kiriview::OpenedCollectionScopeKind::Directory);
    QCOMPARE(unsupportedSession->openedCollectionScope().rootUrl(), directoryCollection->rootUrl());
    QVERIFY(dataLoader.empty());
}

void TestImageLoader::explicitKioArchiveImageStaysImageUrlMode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> decodedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession](kiriview::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/02.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl));

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
    std::optional<kiriview::ImageLoadSession> unsupportedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.unsupportedOpenedCollectionVideo
        = [&unsupportedSession](
              kiriview::ImageLoadSession session) { unsupportedSession = std::move(session); };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl videoUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.bin"));

    loader.start(kiriview::ImageLoadRequest::fromTarget(
        kiriview::ImageDocumentPageTarget {
            videoUrl,
            kiriview::ImageDocumentPageKind::Video,
        },
        *archiveCollection));

    QVERIFY(unsupportedSession.has_value());
    QCOMPARE(unsupportedSession->imageUrl(), videoUrl);
    QCOMPARE(unsupportedSession->kind(), kiriview::ImageDocumentPageKind::Video);
    QCOMPARE(unsupportedSession->location().openedCollectionScopeRootUrl(),
        archiveCollection->rootUrl());
    QVERIFY(dataLoader.empty());
}

void TestImageLoader::archiveInteriorImageKeepsComicBookRoot()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> decodedSession;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedSession](kiriview::ImageLoadSession session, auto) {
        decodedSession = std::move(session);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("02.png"));

    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    loader.start(kiriview::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, imageUrl);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decodedSession.has_value());
    QCOMPARE(decodedSession->location().imageUrl(), imageUrl);
    QCOMPARE(decodedSession->location().openedCollectionScopeRootUrl(), *archiveRootUrl);
}

void TestImageLoader::loadErrorPreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> errorSession;
    std::optional<kiriview::ImageLoadFailure> failure;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &failure](kiriview::ImageLoadSession session,
                          kiriview::ImageLoadFailure loadFailure) {
        errorSession = std::move(session);
        failure = std::move(loadFailure);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/missing.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl));
    dataLoader.failBackLoad(QStringLiteral("KIO data load failed"));

    QTRY_VERIFY(failure.has_value());
    QVERIFY(errorSession.has_value());
    QCOMPARE(errorSession->imageUrl(), imageUrl);
    QCOMPARE(failure->sourceUrl, imageUrl);
    QCOMPARE(failure->sessionId, errorSession->id());
    QVERIFY(failure->kind == kiriview::ImageLoadFailureKind::DataLoad);
    QCOMPARE(failure->userMessage, QStringLiteral("KIO data load failed"));
    QCOMPARE(failure->diagnosticDetail, QStringLiteral("KIO data load failed"));
    QVERIFY(failure->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!failure->retryable);
}

void TestImageLoader::decodeFailurePreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> errorSession;
    std::optional<kiriview::ImageLoadFailure> failure;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &failure](kiriview::ImageLoadSession session,
                          kiriview::ImageLoadFailure loadFailure) {
        errorSession = std::move(session);
        failure = std::move(loadFailure);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl imageUrl = localUrl(QStringLiteral("/images/bad.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(imageUrl));
    dataLoader.finishBackLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(failure.has_value());
    QVERIFY(errorSession.has_value());
    QCOMPARE(failure->sourceUrl, imageUrl);
    QCOMPARE(failure->sessionId, errorSession->id());
    QVERIFY(failure->kind == kiriview::ImageLoadFailureKind::Decode);
    QCOMPARE(failure->userMessage, testImageDecodeFailureString());
    QCOMPARE(failure->diagnosticDetail, testImageDecodeFailureString());
    QVERIFY(failure->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!failure->retryable);
}

void TestImageLoader::openedCollectionCandidateFailurePreservesTypedFailureMetadata()
{
    ManualOpenedCollectionCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> errorSession;
    std::optional<kiriview::ImageLoadFailure> failure;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &failure](kiriview::ImageLoadSession session,
                          kiriview::ImageLoadFailure loadFailure) {
        errorSession = std::move(session);
        failure = std::move(loadFailure);
    };
    kiriview::ImageLoader loader(this, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/missing.cbz"));
    const QString diagnosticDetail = QStringLiteral("opened collection listing failed");
    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl));
    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    candidateProvider.failFrontLoad(diagnosticDetail);

    QVERIFY(failure.has_value());
    QVERIFY(errorSession.has_value());
    QCOMPARE(failure->sourceUrl, archiveUrl);
    QCOMPARE(failure->sessionId, errorSession->id());
    QVERIFY(failure->kind == kiriview::ImageLoadFailureKind::OpenedCollectionLoad);
    QCOMPARE(failure->userMessage, diagnosticDetail);
    QCOMPARE(failure->diagnosticDetail, diagnosticDetail);
    QVERIFY(failure->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!failure->retryable);
}

void TestImageLoader::emptyOpenedCollectionPreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::optional<kiriview::ImageLoadSession> errorSession;
    std::optional<kiriview::ImageLoadFailure> failure;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.error = [&errorSession, &failure](kiriview::ImageLoadSession session,
                          kiriview::ImageLoadFailure loadFailure) {
        errorSession = std::move(session);
        failure = std::move(loadFailure);
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/empty.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl, {});

    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl));

    QVERIFY(failure.has_value());
    QVERIFY(errorSession.has_value());
    QCOMPARE(failure->sourceUrl, archiveUrl);
    QCOMPARE(failure->sessionId, errorSession->id());
    QVERIFY(failure->kind == kiriview::ImageLoadFailureKind::EmptyOpenedCollection);
    QCOMPARE(failure->userMessage, QString());
    QCOMPARE(failure->diagnosticDetail, QString());
    QVERIFY(failure->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!failure->retryable);
}

void TestImageLoader::staleLoadResultIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<QUrl> decodedUrls;
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.decodedImage = [&decodedUrls](kiriview::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.imageUrl());
    };
    kiriview::ImageLoader loader
        = createLoader(this, candidateProvider, dataLoader, std::move(callbacks));

    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    loader.start(kiriview::ImageLoadRequest::fromUrl(firstUrl));
    loader.start(kiriview::ImageLoadRequest::fromUrl(secondUrl));

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
    kiriview::ImageLoader::Callbacks callbacks;
    callbacks.sourceResolved = [&decodedUrls](kiriview::ImageLoadSession session) {
        decodedUrls.push_back(session.imageUrl());
    };
    callbacks.decodedImage = [&decodedUrls](kiriview::ImageLoadSession session, auto) {
        decodedUrls.push_back(session.imageUrl());
    };
    kiriview::ImageLoader loader(this, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        std::move(callbacks));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl staleImageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    const QUrl replacementUrl = localUrl(QStringLiteral("/images/replacement.png"));

    loader.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl));
    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));

    loader.start(kiriview::ImageLoadRequest::fromUrl(replacementUrl));
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
