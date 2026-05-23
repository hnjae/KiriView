// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "candidate_test_support.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "image_async_test_support.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagecandidateprovider.h"
#include "navigation/medianavigationmodel.h"

#include <QImage>
#include <QObject>
#include <QSignalSpy>
#include <QSizeF>
#include <QTemporaryDir>
#include <QTest>
#include <cstddef>
#include <map>
#include <memory>
#include <utility>

namespace {
QString keyForUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments).toString(); }

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::MediaNavigationCandidate mediaCandidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

class FakeMediaCandidateProvider
{
public:
    void setMedia(const QUrl &parentUrl, std::vector<KiriView::MediaNavigationCandidate> candidates)
    {
        m_candidates[keyForUrl(parentUrl)] = std::move(candidates);
    }

    KiriView::MediaNavigationCandidateProvider provider()
    {
        return KiriView::MediaNavigationCandidateProvider {
            [this](QObject *, QUrl parentUrl, KiriView::MediaCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                const auto candidates = m_candidates.find(keyForUrl(parentUrl));
                if (candidates == m_candidates.cend()) {
                    if (errorCallback) {
                        errorCallback(QStringLiteral("missing media candidates"));
                    }
                    return KiriView::ImageIoJob();
                }

                if (callback) {
                    callback(candidates->second);
                }
                return KiriView::ImageIoJob();
            },
        };
    }

private:
    std::map<QString, std::vector<KiriView::MediaNavigationCandidate>> m_candidates;
};

struct ManualMediaCandidateLoad {
    QObject *object = nullptr;
    QUrl parentUrl;
    KiriView::MediaCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualMediaCandidateProvider
{
public:
    KiriView::MediaNavigationCandidateProvider provider()
    {
        return KiriView::MediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl, KiriView::MediaCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualMediaCandidateLoad>();
                load->parentUrl = std::move(parentUrl);
                load->callback = std::move(callback);
                load->errorCallback = std::move(errorCallback);

                KiriView::ImageIoJob job
                    = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
                m_loads.push_back(load);
                return job;
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualMediaCandidateLoad &loadAt(std::size_t index) { return *m_loads.at(index); }

    void finishLoad(std::size_t index, std::vector<KiriView::MediaNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_loads.at(index),
            [candidates = std::move(candidates)](ManualMediaCandidateLoad &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<KiriView::MediaNavigationCandidate> candidates)
    {
        ManualMediaCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualMediaCandidateLoad>> m_loads;
};

std::unique_ptr<KiriDocumentSession> createSessionWithProvider(
    KiriView::MediaNavigationCandidateProvider mediaCandidateProvider,
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr,
    KiriView::ImageNavigationCandidateProvider imageCandidateProvider = {},
    KiriView::ImageDataDecoder imageDataDecoder = KiriView::TestSupport::staticImageDataDecoder())
{
    KiriView::DocumentSessionRuntimeDependencies dependencies;
    dependencies.mediaCandidateProvider = std::move(mediaCandidateProvider);
    dependencies.imageDocumentDependencies.candidateProvider = std::move(imageCandidateProvider);
    if (fileOperations != nullptr) {
        dependencies.fileOperationProvider
            = KiriView::TestSupport::fileOperationProviderFor(*fileOperations);
        dependencies.imageDocumentDependencies.fileOperations
            = KiriView::TestSupport::fileOperationProviderFor(*fileOperations);
    }
    if (imageDataLoader != nullptr) {
        dependencies.imageDocumentDependencies.imageDecode
            = KiriView::TestSupport::imageDecodeDependenciesFor(
                *imageDataLoader, std::move(imageDataDecoder));
    }
    return std::make_unique<KiriDocumentSession>(std::move(dependencies));
}

std::unique_ptr<KiriDocumentSession> createSession(FakeMediaCandidateProvider &mediaProvider,
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr)
{
    return createSessionWithProvider(mediaProvider.provider(), fileOperations, imageDataLoader);
}

void compareUnavailableActiveNavigation(const KiriDocumentSession &session)
{
    QVERIFY(!session.activeNavigationAvailable());
    QVERIFY(!session.activeNavigationKnown());
    QVERIFY(!session.activeNavigationEditable());
    QCOMPARE(session.activeNavigationCurrentNumber(), 0);
    QCOMPARE(session.activeNavigationCount(), 0);
    QVERIFY(!session.canOpenPreviousActiveNavigation());
    QVERIFY(!session.canOpenNextActiveNavigation());
    QVERIFY(!session.atKnownFirstActiveNavigation());
    QVERIFY(!session.atKnownLastActiveNavigation());
}

bool writeTestImage(const QString &path)
{
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    return image.save(path, "PNG");
}
}

class TestKiriDocumentSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySessionProjectsUnavailableActiveNavigation();
    void directVideoRoutesToVideoDocumentWithOriginalSource();
    void activeZoomReadoutFollowsSessionDocumentKind();
    void archiveAndDirectoryInputsRouteToImageDocument();
    void directImageAfterVideoRestoresImageDocument();
    void directImageMediaNavigationIncludesSiblingVideos();
    void freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl();
    void directImageReplacementFailureRestoresPreviousMediaCursor();
    void stalePendingDirectImageCandidateCompletionCannotPublishForNewCursor();
    void freshDirectImageFailureLeavesNavigationUnknown();
    void archiveImageDocumentProjectsActiveNavigationFromPages();
    void imagePageNavigationChangesEmitActiveNavigationWhenRelevant();
    void activeNavigationNumberDispatchRoutesDirectMedia();
    void activeNavigationNumberDispatchRoutesImageDocumentPages();
    void activeNavigationNumberDispatchIgnoresUnknownNavigation();
    void activeNavigationClearsWhenSwitchingFromKnownDirectMedia();
    void activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount();
    void twoPageSpreadLastBoundaryProjectsThroughActiveNavigation();
    void videoNavigationKeepsStillImagePredecodeCache();
    void videoMediaNavigationExposesCurrentNumberAndCount();
    void initialDirectImagePredecodeUsesRequestedMediaCursor();
    void staleMediaCandidateCompletionCannotPublishForNewSource();
    void nextMediaFromVideoCanRouteToImageWithoutUsingImageNavigation();
    void nonMediaImageDeletionProgressIsMirroredThroughSessionState();
    void directImageDeletionCanOpenVideoFallback();
    void pendingDirectImageReplacementDoesNotDeletePreviousDisplayedFile();
    void videoDeletionUsesOriginalUrlAndOpensMediaFallback();
    void canceledVideoDeletionKeepsCurrentVideo();
    void staleVideoDeletionCompletionAfterSourceChangeIsIgnored();
};

void TestKiriDocumentSession::emptySessionProjectsUnavailableActiveNavigation()
{
    FakeMediaCandidateProvider mediaProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::directVideoRoutesToVideoDocumentWithOriginalSource()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/clip.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")), { mediaCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(clip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), clip);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);
    QCOMPARE(session->imageDocument()->sourceUrl(), QUrl());
    QVERIFY(session->mediaNavigationActive());
    QVERIFY(session->atKnownFirstMedia());
    QVERIFY(session->atKnownLastMedia());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeZoomReadoutFollowsSessionDocumentKind()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeMediaCandidateProvider mediaProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    mediaProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Empty);
    QVERIFY(!session->activeZoomPercentAvailable());
    QVERIFY(!session->activeZoomPercentKnown());
    QCOMPARE(session->activeZoomPercent(), 0.0);
    QVERIFY(!session->activeZoomEditable());

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeZoomPercentAvailable());
    QVERIFY(!session->activeZoomPercentKnown());
    QCOMPARE(session->activeZoomPercent(), 0.0);
    QVERIFY(!session->activeZoomEditable());

    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));
    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->activeZoomPercentAvailable());
    QVERIFY(session->activeZoomPercentKnown());
    QVERIFY(session->activeZoomPercent() > 0.0);
    QVERIFY(session->activeZoomEditable());
}

void TestKiriDocumentSession::archiveAndDirectoryInputsRouteToImageDocument()
{
    FakeMediaCandidateProvider mediaProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    const QUrl archive = localUrl(QStringLiteral("/books/book.zip"));
    session->setSourceUrl(archive);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->imageDocument()->sourceUrl(), archive);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());

    const QUrl directory = localUrl(QStringLiteral("/books/"));
    session->setSourceUrl(directory);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->imageDocument()->sourceUrl(), directory);
}

void TestKiriDocumentSession::directImageAfterVideoRestoresImageDocument()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.svg"));
    mediaProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { mediaCandidate(clip), mediaCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(clip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);

    session->setSourceUrl(image);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), image);
    QCOMPARE(session->imageDocument()->sourceUrl(), image);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::directImageMediaNavigationIncludesSiblingVideos()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeMediaCandidateProvider mediaProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    mediaProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QTRY_VERIFY(session->mediaNavigationActive());
    QTRY_VERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QCOMPARE(session->mediaCount(), 2);
    QVERIFY(!session->canOpenPreviousMedia());
    QVERIFY(session->canOpenNextMedia());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());

    session->openMediaAtNumber(2);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
    QCOMPARE(session->currentMediaNumber(), 2);
    QCOMPARE(session->mediaCount(), 2);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Loading);
    QVERIFY(session->mediaNavigationActive());
    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QCOMPARE(session->mediaCount(), 2);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
}

void TestKiriDocumentSession::directImageReplacementFailureRestoresPreviousMediaCursor()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(firstImage), mediaCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QCOMPARE(session->currentMediaNumber(), 1);

    session->setSourceUrl(secondImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 2);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("replacement failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QCOMPARE(session->mediaCount(), 2);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
}

void TestKiriDocumentSession::stalePendingDirectImageCandidateCompletionCannotPublishForNewCursor()
{
    ManualMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(mediaProvider.provider(), nullptr, &imageDataLoader);

    const QUrl firstImage = localUrl(QStringLiteral("/first/01.png"));
    const QUrl staleSibling = localUrl(QStringLiteral("/first/02.mp4"));
    const QUrl secondImage = localUrl(QStringLiteral("/second/01.png"));

    session->setSourceUrl(firstImage);
    QCOMPARE(mediaProvider.loadCount(), std::size_t(1));
    QCOMPARE(mediaProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/first/")));

    session->setSourceUrl(secondImage);
    QCOMPARE(mediaProvider.loadCount(), std::size_t(2));
    QCOMPARE(mediaProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/second/")));
    QVERIFY(mediaProvider.loadAt(0).canceled);
    QVERIFY(!session->mediaNavigationKnown());

    mediaProvider.deliverIgnoringCancellation(
        0, { mediaCandidate(firstImage), mediaCandidate(staleSibling) });

    QVERIFY(!session->mediaNavigationKnown());
    QCOMPARE(session->activeNavigationCount(), 0);

    mediaProvider.finishLoad(1, { mediaCandidate(secondImage) });

    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QCOMPARE(session->mediaCount(), 1);
}

void TestKiriDocumentSession::freshDirectImageFailureLeavesNavigationUnknown()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl siblingUrl = localUrl(QStringLiteral("/media/02.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(imageUrl), mediaCandidate(siblingUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("initial load failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Error);
    QVERIFY(!session->mediaNavigationKnown());
    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
}

void TestKiriDocumentSession::archiveImageDocumentProjectsActiveNavigationFromPages()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider imageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("02.png"));
    imageCandidates.setArchiveImages(archiveDocument->rootUrl(),
        { KiriView::TestSupport::imageCandidate(firstPage),
            KiriView::TestSupport::imageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        mediaProvider.provider(), nullptr, &dataLoader, imageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(!session->mediaNavigationActive());
    QVERIFY(session->activeNavigationAvailable());
    QTRY_COMPARE(session->imageDocument()->currentPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->currentLastPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->imageCount(), 2);
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::imagePageNavigationChangesEmitActiveNavigationWhenRelevant()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider imageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/signals.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("02.png"));
    imageCandidates.setArchiveImages(archiveDocument->rootUrl(),
        { KiriView::TestSupport::imageCandidate(firstPage),
            KiriView::TestSupport::imageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        mediaProvider.provider(), nullptr, &dataLoader, imageCandidates.provider());
    QSignalSpy activeNavigationSpy(session.get(), &KiriDocumentSession::activeNavigationChanged);

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QTRY_COMPARE(session->activeNavigationCount(), 2);
    const int baselineSignalCount = activeNavigationSpy.count();

    session->imageDocument()->openImageAtPage(2);

    QTRY_COMPARE(session->activeNavigationCurrentNumber(), 2);
    QVERIFY(activeNavigationSpy.count() > baselineSignalCount);
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeNavigationNumberDispatchRoutesDirectMedia()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);

    session->openActiveNavigationAtNumber(1);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
}

void TestKiriDocumentSession::activeNavigationNumberDispatchRoutesImageDocumentPages()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider imageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/number-dispatch.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("02.png"));
    imageCandidates.setArchiveImages(archiveDocument->rootUrl(),
        { KiriView::TestSupport::imageCandidate(firstPage),
            KiriView::TestSupport::imageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        mediaProvider.provider(), nullptr, &dataLoader, imageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);

    session->openActiveNavigationAtNumber(2);

    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(session->imageDocument()->currentPageNumber(), 2);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
}

void TestKiriDocumentSession::activeNavigationNumberDispatchIgnoresUnknownNavigation()
{
    ManualMediaCandidateProvider mediaProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(mediaProvider.provider());

    session->setSourceUrl(first);

    QCOMPARE(mediaProvider.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(mediaProvider.loadCount(), std::size_t(1));
    QCOMPARE(session->sourceUrl(), first);

    mediaProvider.finishLoad(0, { mediaCandidate(first), mediaCandidate(second) });
    QVERIFY(session->activeNavigationKnown());
    session->openActiveNavigationAtNumber(2);
    QCOMPARE(mediaProvider.loadCount(), std::size_t(2));
    mediaProvider.finishLoad(1, { mediaCandidate(first), mediaCandidate(second) });
    QCOMPARE(session->sourceUrl(), second);
}

void TestKiriDocumentSession::activeNavigationClearsWhenSwitchingFromKnownDirectMedia()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider imageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/clear.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl page = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    imageCandidates.setArchiveImages(
        archiveDocument->rootUrl(), { KiriView::TestSupport::imageCandidate(page) });
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl lastClip = localUrl(QStringLiteral("/media/03.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(clip), mediaCandidate(nextClip), mediaCandidate(lastClip) });
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        mediaProvider.provider(), nullptr, &dataLoader, imageCandidates.provider());

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);

    session->setSourceUrl(archiveUrl);

    QVERIFY(session->activeNavigationCount() != 3);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("page"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);

    session->setSourceUrl(QUrl());

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Empty);
    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl third = localUrl(QStringLiteral("/media/03.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(first), mediaCandidate(second), mediaCandidate(third) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(second);

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());

    session->openLastActiveNavigation();

    QCOMPARE(session->sourceUrl(), third);
    QCOMPARE(session->activeNavigationCurrentNumber(), 3);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::twoPageSpreadLastBoundaryProjectsThroughActiveNavigation()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::FakeImageNavigationCandidateProvider imageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPage = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("03.png"));
    imageCandidates.setArchiveImages(archiveDocument->rootUrl(),
        { KiriView::TestSupport::imageCandidate(firstPage),
            KiriView::TestSupport::imageCandidate(secondPage),
            KiriView::TestSupport::imageCandidate(thirdPage) });
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        mediaProvider.provider(), nullptr, &dataLoader, imageCandidates.provider(),
        KiriView::TestSupport::staticImageDataDecoder(
            KiriView::TestSupport::testImage(QSize(100, 200))));
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    session->imageDocument()->setTwoPageModeEnabled(true);
    session->imageDocument()->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("third"));

    QTRY_VERIFY(session->imageDocument()->secondaryPageVisible());
    QCOMPARE(session->imageDocument()->currentPageNumber(), 2);
    QCOMPARE(session->imageDocument()->currentLastPageNumber(), 3);
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::videoNavigationKeepsStillImagePredecodeCache()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl video = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(firstImage), mediaCandidate(video), mediaCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    imageDataLoader.finishFrontLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
    imageDataLoader.finishBackLoad(QByteArrayLiteral("next"));

    session->openNextMedia();
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), video);

    const std::size_t loadCountBeforeReturn = imageDataLoader.loadCount();
    session->openPreviousMedia();

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(imageDataLoader.loadCount(), loadCountBeforeReturn);
}

void TestKiriDocumentSession::videoMediaNavigationExposesCurrentNumberAndCount()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->mediaNavigationActive());
    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 2);
    QCOMPARE(session->mediaCount(), 2);
    QVERIFY(session->canOpenPreviousMedia());
    QVERIFY(!session->canOpenNextMedia());

    session->openMediaAtNumber(1);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::initialDirectImagePredecodeUsesRequestedMediaCursor()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl video = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/03.png"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(firstImage), mediaCandidate(video), mediaCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
}

void TestKiriDocumentSession::staleMediaCandidateCompletionCannotPublishForNewSource()
{
    ManualMediaCandidateProvider mediaProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(mediaProvider.provider());

    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mp4"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mp4"));
    const QUrl secondSibling = localUrl(QStringLiteral("/second/02.mp4"));

    session->setSourceUrl(firstClip);
    QCOMPARE(mediaProvider.loadCount(), std::size_t(1));
    QCOMPARE(mediaProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/first/")));

    session->setSourceUrl(secondClip);
    QCOMPARE(mediaProvider.loadCount(), std::size_t(2));
    QCOMPARE(mediaProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/second/")));
    QVERIFY(mediaProvider.loadAt(0).canceled);
    QVERIFY(!session->mediaNavigationKnown());

    mediaProvider.deliverIgnoringCancellation(
        0, { mediaCandidate(secondClip), mediaCandidate(secondSibling) });

    QVERIFY(!session->mediaNavigationKnown());

    mediaProvider.finishLoad(1, { mediaCandidate(secondClip) });

    QVERIFY(session->mediaNavigationKnown());
    QCOMPARE(session->currentMediaNumber(), 1);
    QCOMPARE(session->mediaCount(), 1);
}

void TestKiriDocumentSession::nextMediaFromVideoCanRouteToImageWithoutUsingImageNavigation()
{
    FakeMediaCandidateProvider mediaProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.png"));
    mediaProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { mediaCandidate(clip), mediaCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider);
    session->setSourceUrl(clip);

    QVERIFY(session->canOpenNextMedia());
    session->openNextMedia();

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), image);
    QCOMPARE(session->imageDocument()->sourceUrl(), image);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::nonMediaImageDeletionProgressIsMirroredThroughSessionState()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("page.png"));
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl directoryUrl = localUrl(directory.path());
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider, &fileOperations);
    QSignalSpy progressSpy(session.get(), &KiriDocumentSession::fileDeletionInProgressChanged);

    session->setSourceUrl(directoryUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(!session->mediaNavigationActive());

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, directoryUrl);
    QVERIFY(session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 1);

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 2);
}

void TestKiriDocumentSession::directImageDeletionCanOpenVideoFallback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    mediaProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { mediaCandidate(imageUrl), mediaCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider, &fileOperations);
    session->setSourceUrl(imageUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->mediaNavigationActive());

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, imageUrl);

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
}

void TestKiriDocumentSession::pendingDirectImageReplacementDoesNotDeletePreviousDisplayedFile()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { mediaCandidate(firstImage), mediaCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(mediaProvider, &fileOperations, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);

    session->setSourceUrl(secondImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(!session->displayedFileDeletionAvailable());
    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);
    QCOMPARE(fileOperations.operationCount(), std::size_t(0));

    imageDataLoader.failBackLoad(QStringLiteral("replacement failed"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->displayedFileDeletionAvailable());
    QCOMPARE(session->currentMediaNumber(), 1);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, firstImage);
}

void TestKiriDocumentSession::videoDeletionUsesOriginalUrlAndOpensMediaFallback()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl clip = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/01.mp4"));
    const QUrl fallback = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/02.jpg"));
    mediaProvider.setMedia(QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")),
        { mediaCandidate(clip), mediaCandidate(fallback) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider, &fileOperations);
    session->setSourceUrl(clip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, clip);
    QVERIFY(session->fileDeletionInProgress());

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->sourceUrl(), fallback);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
}

void TestKiriDocumentSession::canceledVideoDeletionKeepsCurrentVideo()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mov"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/media/")), { mediaCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider, &fileOperations);
    session->setSourceUrl(clip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::DeletePermanently);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), clip);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);
    QCOMPARE(session->errorString(), QString());
}

void TestKiriDocumentSession::staleVideoDeletionCompletionAfterSourceChangeIsIgnored()
{
    FakeMediaCandidateProvider mediaProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mov"));
    const QUrl staleFallback = localUrl(QStringLiteral("/first/02.png"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mov"));
    mediaProvider.setMedia(localUrl(QStringLiteral("/first/")),
        { mediaCandidate(firstClip), mediaCandidate(staleFallback) });
    mediaProvider.setMedia(localUrl(QStringLiteral("/second/")), { mediaCandidate(secondClip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(mediaProvider, &fileOperations);
    session->setSourceUrl(firstClip);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.operationAt(0).request.targetUrl, firstClip);
    QVERIFY(session->fileDeletionInProgress());

    session->setSourceUrl(secondClip);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(!session->fileDeletionInProgress());
    QVERIFY(fileOperations.operationAt(0).canceled);

    fileOperations.deliverOperationAtIgnoringCancellation(
        0, KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), secondClip);
    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->errorString(), QString());
}

QTEST_GUILESS_MAIN(TestKiriDocumentSession)

#include "test_kiridocumentsession.moc"
