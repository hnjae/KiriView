// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "candidate_test_support.h"
#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "image_async_test_support.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/directmedianavigationmodel.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "session/activenavigationthumbnailmodel.h"

#include <QAbstractItemModel>
#include <QFile>
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

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

QVariant thumbnailData(const KiriDocumentSession &session, int row, int role)
{
    QAbstractItemModel *model = session.activeNavigationThumbnailModel();
    return model->data(model->index(row, 0), role);
}

void compareThumbnailRow(const KiriDocumentSession &session, int row, int number, const QUrl &url,
    const QString &label, const QString &iconName, bool current)
{
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::NumberRole).toInt(),
        number);
    QCOMPARE(thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::UrlRole).toUrl(),
        url);
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::LabelRole).toString(),
        label);
    QCOMPARE(thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::IconNameRole)
                 .toString(),
        iconName);
    QCOMPARE(
        thumbnailData(session, row, KiriView::ActiveNavigationThumbnailModel::CurrentRole).toBool(),
        current);
}

class FakeDirectMediaNavigationCandidateProvider
{
public:
    void setMedia(
        const QUrl &parentUrl, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        m_candidates[keyForUrl(parentUrl)] = std::move(candidates);
    }

    KiriView::DirectMediaNavigationCandidateProvider provider()
    {
        return KiriView::DirectMediaNavigationCandidateProvider {
            [this](QObject *, QUrl parentUrl,
                KiriView::DirectMediaNavigationCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                const auto candidates = m_candidates.find(keyForUrl(parentUrl));
                if (candidates == m_candidates.cend()) {
                    if (errorCallback) {
                        errorCallback(QStringLiteral("missing direct media candidates"));
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
    std::map<QString, std::vector<KiriView::DirectMediaNavigationCandidate>> m_candidates;
};

struct ManualDirectMediaNavigationCandidateLoad {
    QObject *object = nullptr;
    QUrl parentUrl;
    KiriView::DirectMediaNavigationCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDirectMediaNavigationCandidateProvider
{
public:
    KiriView::DirectMediaNavigationCandidateProvider provider()
    {
        return KiriView::DirectMediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl,
                KiriView::DirectMediaNavigationCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualDirectMediaNavigationCandidateLoad>();
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

    ManualDirectMediaNavigationCandidateLoad &loadAt(std::size_t index)
    {
        return *m_loads.at(index);
    }

    void finishLoad(
        std::size_t index, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_loads.at(index),
            [candidates = std::move(candidates)](
                ManualDirectMediaNavigationCandidateLoad &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        ManualDirectMediaNavigationCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualDirectMediaNavigationCandidateLoad>> m_loads;
};

class FakeMediaOpenWithProvider
{
public:
    KiriView::MediaOpenWithProvider provider()
    {
        return [this](QObject *, KiriView::MediaOpenWithRequest request,
                   KiriView::MediaOpenWithCallback callback) {
            requests.push_back(std::move(request));
            if (callback) {
                callback(result, errorString);
            }
            return KiriView::ImageIoJob();
        };
    }

    std::vector<KiriView::MediaOpenWithRequest> requests;
    KiriView::MediaOpenWithResult result = KiriView::MediaOpenWithResult::Succeeded;
    QString errorString;
};

std::unique_ptr<KiriDocumentSession> createSessionWithProvider(
    KiriView::DirectMediaNavigationCandidateProvider directMediaNavigationCandidateProvider,
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr,
    KiriView::ImageDocumentPageCandidateProvider imageDocumentPageCandidateProvider = {},
    KiriView::ImageDataDecoder imageDataDecoder = KiriView::TestSupport::staticImageDataDecoder(),
    KiriView::MediaOpenWithProvider mediaOpenWithProvider = {})
{
    KiriView::DocumentSessionRuntimeDependencies dependencies;
    dependencies.directMediaNavigationCandidateProvider
        = std::move(directMediaNavigationCandidateProvider);
    dependencies.mediaOpenWithProvider = std::move(mediaOpenWithProvider);
    dependencies.imageDocumentDependencies.candidateProvider
        = std::move(imageDocumentPageCandidateProvider);
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

std::unique_ptr<KiriDocumentSession> createSession(
    FakeDirectMediaNavigationCandidateProvider &directMediaNavigationProvider,
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr)
{
    return createSessionWithProvider(
        directMediaNavigationProvider.provider(), fileOperations, imageDataLoader);
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

bool writeEmptyFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly);
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
    void kioArchiveImageAfterKioArchiveVideoUsesOriginalImageUrl();
    void directImageDirectMediaNavigationIncludesSiblingVideos();
    void directImageActiveNavigationIgnoresImageDocumentDirectoryPageCandidates();
    void directArchiveEntryImageUsesDirectMediaNavigationWithoutImageDocumentPages();
    void directMediaThumbnailModelTracksSiblingCandidates();
    void directMediaThumbnailModelStaysEmptyUntilCandidatesAreKnown();
    void defaultMediaProviderListsLocalDirectImageSiblings();
    void defaultMediaProviderListsLocalDirectVideoSiblings();
    void freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl();
    void directImageDocumentPageCandidateCompletionSurvivesCursorConfirmation();
    void directImageReplacementFailureRestoresPreviousMediaCursor();
    void stalePendingDirectImageDocumentPageCandidateCompletionCannotPublishForNewCursor();
    void freshDirectImageFailureLeavesNavigationUnknown();
    void archiveImageDocumentProjectsActiveNavigationFromPages();
    void imageDocumentPageNavigationChangesEmitActiveNavigationWhenRelevant();
    void activeNavigationNumberDispatchRoutesDirectMedia();
    void activeNavigationNumberDispatchRoutesImageDocumentPages();
    void archiveCollectionThumbnailModelUsesPageCandidateNames();
    void activeNavigationRequestReportsDispatchAndBoundaryResults();
    void activeNavigationBoundaryTextFollowsSessionSource();
    void activeNavigationNumberDispatchIgnoresUnknownNavigation();
    void activeNavigationClearsWhenSwitchingFromKnownDirectMedia();
    void activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount();
    void activeNavigationBoundaryScopeFollowsSessionSource();
    void openWithIsUnavailableInEmptySession();
    void openWithUsesCurrentDirectImageUrl();
    void openWithFailureEmitsToastSignal();
    void twoPageSpreadLastBoundaryProjectsThroughActiveNavigation();
    void videoNavigationKeepsStillImagePredecodeCache();
    void videoActiveNavigationExposesCurrentNumberAndCount();
    void initialDirectImagePredecodeUsesRequestedMediaCursor();
    void directImagePredecodeDoesNotUseImageDocumentPageCandidates();
    void staleDirectMediaNavigationCandidateCompletionCannotPublishForNewSource();
    void nextMediaFromVideoCanRouteToImageWithoutUsingImageDocumentPageNavigation();
    void nonMediaImageDeletionProgressIsMirroredThroughSessionState();
    void directMediaDeletionInProgressDisablesActiveNavigationDispatch();
    void directImageDeletionCanOpenVideoFallback();
    void pendingDirectImageReplacementDoesNotDeletePreviousDisplayedFile();
    void pendingDirectMediaDeletionCandidateLoadIsCanceledBySourceChange();
    void videoDeletionUsesOriginalUrlAndOpensMediaFallback();
    void canceledVideoDeletionKeepsCurrentVideo();
    void staleVideoDeletionCompletionAfterSourceChangeIsIgnored();
};

void TestKiriDocumentSession::emptySessionProjectsUnavailableActiveNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::directVideoRoutesToVideoDocumentWithOriginalSource()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/clip.mp4"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), clip);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("clip.mp4"));
    QCOMPARE(session->imageDocument()->sourceUrl(), QUrl());
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

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.svg"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(clip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), clip);

    session->setSourceUrl(image);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), image);
    QCOMPARE(session->imageDocument()->sourceUrl(), image);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::kioArchiveImageAfterKioArchiveVideoUsesOriginalImageUrl()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl parentUrl(QStringLiteral("zip:///books/book.zip!/chapter/"));
    const QUrl videoUrl(QStringLiteral("zip:///books/book.zip!/chapter/clip.mp4"));
    const QUrl imageUrl(QStringLiteral("zip:///books/book.zip!/chapter/page.png"));
    directMediaNavigationProvider.setMedia(parentUrl, { directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader);

    session->setSourceUrl(videoUrl);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);

    directMediaNavigationProvider.setMedia(parentUrl, { directMediaNavigationCandidate(imageUrl) });
    session->setSourceUrl(imageUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, imageUrl);
    QVERIFY(dataLoader.backLoad().openedCollectionScope.isEmpty());
    QVERIFY(!dataLoader.backLoad().url.isLocalFile());
    QVERIFY(!dataLoader.backLoad().url.toString().contains(QStringLiteral("kio-fuse")));
}

void TestKiriDocumentSession::directImageDirectMediaNavigationIncludesSiblingVideos()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("01.png – 2×2"));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("02.mp4"));
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::
    directImageActiveNavigationIgnoresImageDocumentDirectoryPageCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl extraImageUrl = localUrl(QStringLiteral("/media/03.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    imageDocumentPageCandidates.setDirectoryImages(localUrl(QStringLiteral("/media/")),
        { KiriView::TestSupport::imageDocumentPageCandidate(imageUrl),
            KiriView::ImageDocumentPageCandidate {
                videoUrl, QStringLiteral("02.mp4"), KiriView::ImageDocumentPageKind::Video },
            KiriView::TestSupport::imageDocumentPageCandidate(extraImageUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->imageDocument()->currentPageNumber(), 0);
    QCOMPARE(session->imageDocument()->pageCount(), 0);
}

void TestKiriDocumentSession::
    directArchiveEntryImageUsesDirectMediaNavigationWithoutImageDocumentPages()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl directoryUrl(QStringLiteral("zip:///path/archive.zip!/chapter/"));
    const QUrl imageUrl(QStringLiteral("zip:///path/archive.zip!/chapter/01.png"));
    const QUrl videoUrl(QStringLiteral("zip:///path/archive.zip!/chapter/02.mp4"));
    directMediaNavigationProvider.setMedia(directoryUrl,
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    imageDocumentPageCandidates.setDirectoryImages(directoryUrl,
        { KiriView::TestSupport::imageDocumentPageCandidate(imageUrl),
            KiriView::ImageDocumentPageCandidate {
                videoUrl, QStringLiteral("02.mp4"), KiriView::ImageDocumentPageKind::Video } });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->imageDocument()->currentPageNumber(), 0);
    QCOMPARE(session->imageDocument()->pageCount(), 0);
}

void TestKiriDocumentSession::directMediaThumbnailModelTracksSiblingCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 0, 1, imageUrl, QStringLiteral("01.png"),
        QStringLiteral("image-x-generic-symbolic"), false);
    compareThumbnailRow(*session, 1, 2, videoUrl, QStringLiteral("02.mp4"),
        QStringLiteral("video-x-generic-symbolic"), true);
}

void TestKiriDocumentSession::directMediaThumbnailModelStaysEmptyUntilCandidatesAreKnown()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    session->setSourceUrl(videoUrl);

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(model->rowCount(), 0);

    directMediaNavigationProvider.finishLoad(
        0, { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 1, 2, videoUrl, QStringLiteral("02.mp4"),
        QStringLiteral("video-x-generic-symbolic"), true);
}

void TestKiriDocumentSession::defaultMediaProviderListsLocalDirectImageSiblings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstImagePath = directory.filePath(QStringLiteral("01.png"));
    const QString videoPath = directory.filePath(QStringLiteral("02.mp4"));
    const QString currentImagePath = directory.filePath(QStringLiteral("03.png"));
    QVERIFY(writeTestImage(firstImagePath));
    QVERIFY(writeEmptyFile(videoPath));
    QVERIFY(writeTestImage(currentImagePath));

    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(KiriView::DirectMediaNavigationCandidateProvider {});

    session->setSourceUrl(localUrl(currentImagePath));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 3);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::defaultMediaProviderListsLocalDirectVideoSiblings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString firstImagePath = directory.filePath(QStringLiteral("01.png"));
    const QString currentVideoPath = directory.filePath(QStringLiteral("02.mp4"));
    const QString nextVideoPath = directory.filePath(QStringLiteral("03.mov"));
    QVERIFY(writeTestImage(firstImagePath));
    QVERIFY(writeEmptyFile(currentVideoPath));
    QVERIFY(writeEmptyFile(nextVideoPath));

    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(KiriView::DirectMediaNavigationCandidateProvider {});

    session->setSourceUrl(localUrl(currentVideoPath));

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->videoDocument()->sourceUrl(), localUrl(currentVideoPath));
    QVERIFY(session->activeNavigationAvailable());
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::freshDirectImageReadoutUsesRequestedCursorBeforeDisplayedUrl()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QVERIFY(session->activeNavigationAvailable());
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Loading);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
}

void TestKiriDocumentSession::directImageDocumentPageCandidateCompletionSurvivesCursorConfirmation()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl siblingUrl = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());

    imageDataLoader.finishBackLoad(QByteArrayLiteral("image"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), imageUrl);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(!directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());

    directMediaNavigationProvider.finishLoad(0,
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(siblingUrl) });

    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->canOpenNextActiveNavigation());
}

void TestKiriDocumentSession::directImageReplacementFailureRestoresPreviousMediaCursor()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    imageDataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());

    session->setSourceUrl(secondImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QCOMPARE(session->sourceUrl(), secondImage);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("replacement failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), firstImage);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::
    stalePendingDirectImageDocumentPageCandidateCompletionCannotPublishForNewCursor()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    std::unique_ptr<KiriDocumentSession> session = createSessionWithProvider(
        directMediaNavigationProvider.provider(), nullptr, &imageDataLoader);

    const QUrl firstImage = localUrl(QStringLiteral("/first/01.png"));
    const QUrl staleSibling = localUrl(QStringLiteral("/first/02.mp4"));
    const QUrl secondImage = localUrl(QStringLiteral("/second/01.png"));

    session->setSourceUrl(firstImage);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/first/")));

    session->setSourceUrl(secondImage);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/second/")));
    QVERIFY(directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(!session->activeNavigationKnown());

    directMediaNavigationProvider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(staleSibling) });

    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCount(), 0);

    directMediaNavigationProvider.finishLoad(1, { directMediaNavigationCandidate(secondImage) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
}

void TestKiriDocumentSession::freshDirectImageFailureLeavesNavigationUnknown()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl siblingUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(siblingUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(imageUrl);
    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);

    imageDataLoader.failBackLoad(QStringLiteral("initial load failed"));

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Error);
    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
}

void TestKiriDocumentSession::archiveImageDocumentProjectsActiveNavigationFromPages()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));

    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);
    QVERIFY(session->activeNavigationAvailable());
    QTRY_COMPARE(session->imageDocument()->currentPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->currentLastPageNumber(), 1);
    QTRY_COMPARE(session->imageDocument()->pageCount(), 2);
    QTRY_VERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("book.cbz – 1/2"));
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::imageDocumentPageNavigationChangesEmitActiveNavigationWhenRelevant()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/signals.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());
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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/number-dispatch.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

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

void TestKiriDocumentSession::archiveCollectionThumbnailModelUsesPageCandidateNames()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/thumbnails.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("chapter/01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("extras/clip.mp4"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            KiriView::ImageDocumentPageCandidate { firstPage, QStringLiteral("chapter/01.png"),
                KiriView::ImageDocumentPageKind::Image },
            KiriView::ImageDocumentPageCandidate { secondPage, QStringLiteral("extras/clip.mp4"),
                KiriView::ImageDocumentPageKind::Video },
        });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));

    QAbstractItemModel *model = session->activeNavigationThumbnailModel();
    QVERIFY(model != nullptr);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    compareThumbnailRow(*session, 0, 1, firstPage, QStringLiteral("chapter/01.png"),
        QStringLiteral("image-x-generic-symbolic"), true);
    compareThumbnailRow(*session, 1, 2, secondPage, QStringLiteral("extras/clip.mp4"),
        QStringLiteral("video-x-generic-symbolic"), false);
}

void TestKiriDocumentSession::activeNavigationRequestReportsDispatchAndBoundaryResults()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/request-results.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->atKnownFirstActiveNavigation());

    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::FirstActiveNavigationBoundary);

    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::ActiveNavigationRequestDispatched);
    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(session->activeNavigationCurrentNumber(), 2);
    QVERIFY(session->atKnownLastActiveNavigation());

    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::LastActiveNavigationBoundary);
}

void TestKiriDocumentSession::activeNavigationBoundaryTextFollowsSessionSource()
{
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/request-boundary-text.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage) });
    FakeDirectMediaNavigationCandidateProvider unusedDirectMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> imageSession
        = createSessionWithProvider(unusedDirectMediaNavigationProvider.provider(), nullptr,
            &dataLoader, imageDocumentPageCandidates.provider());

    imageSession->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(imageSession->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(imageSession->atKnownFirstActiveNavigation());

    QCOMPARE(
        imageSession->requestPreviousActiveNavigationBoundaryText(), QStringLiteral("First image"));
    QCOMPARE(imageSession->requestNextActiveNavigationBoundaryText(), QString());

    QTRY_COMPARE(dataLoader.backLoad().url, secondPage);
    dataLoader.finishBackLoad(QByteArrayLiteral("second"));
    QTRY_COMPARE(imageSession->activeNavigationCurrentNumber(), 2);
    QVERIFY(imageSession->atKnownLastActiveNavigation());

    QCOMPARE(imageSession->requestNextActiveNavigationBoundaryText(), QStringLiteral("Last image"));
}

void TestKiriDocumentSession::activeNavigationNumberDispatchIgnoresUnknownNavigation()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    session->setSourceUrl(first);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(!session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);
    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);

    session->openActiveNavigationAtNumber(2);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(session->sourceUrl(), first);

    directMediaNavigationProvider.finishLoad(
        0, { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    QVERIFY(session->activeNavigationKnown());
    session->openActiveNavigationAtNumber(2);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    directMediaNavigationProvider.finishLoad(
        1, { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second) });
    QCOMPARE(session->sourceUrl(), second);
}

void TestKiriDocumentSession::activeNavigationClearsWhenSwitchingFromKnownDirectMedia()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/clear.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl page = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(
        archiveCollection->rootUrl(), { KiriView::TestSupport::imageDocumentPageCandidate(page) });
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl lastClip = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(nextClip),
            directMediaNavigationCandidate(lastClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("02.mp4"));

    session->setSourceUrl(archiveUrl);

    QVERIFY(session->activeNavigationCount() != 3);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("page"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 1);
    QCOMPARE(session->windowTitleSubject(), QStringLiteral("clear.cbz – 1/1"));

    session->setSourceUrl(nextClip);
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);

    session->setSourceUrl(QUrl());

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Empty);
    QCOMPARE(session->windowTitleSubject(), QString());
    compareUnavailableActiveNavigation(*session);
}

void TestKiriDocumentSession::activeNavigationAvailabilityUsesSameSnapshotAsCurrentAndCount()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl first = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl second = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl third = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(first), directMediaNavigationCandidate(second),
            directMediaNavigationCandidate(third) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

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

    session->openFirstActiveNavigation();

    QCOMPARE(session->sourceUrl(), first);
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());
    QVERIFY(session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
}

void TestKiriDocumentSession::activeNavigationBoundaryScopeFollowsSessionSource()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/boundary.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl page = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(
        archiveCollection->rootUrl(), { KiriView::TestSupport::imageDocumentPageCandidate(page) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider());

    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::NoNavigationBoundary);

    session->setSourceUrl(clip);

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("page"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_VERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);
}

void TestKiriDocumentSession::openWithIsUnavailableInEmptySession()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    FakeMediaOpenWithProvider openWithProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());

    QVERIFY(!session->displayedMediaOpenWithAvailable());

    session->openCurrentMediaWith();

    QVERIFY(openWithProvider.requests.empty());
}

void TestKiriDocumentSession::openWithUsesCurrentDirectImageUrl()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));
    const QUrl imageUrl = localUrl(imagePath);

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeMediaOpenWithProvider openWithProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());

    QSignalSpy availabilitySpy(
        session.get(), &KiriDocumentSession::displayedMediaOpenWithAvailabilityChanged);
    session->setSourceUrl(imageUrl);

    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QVERIFY(session->displayedMediaOpenWithAvailable());
    QVERIFY(availabilitySpy.count() > 0);

    session->openCurrentMediaWith();

    QCOMPARE(openWithProvider.requests.size(), std::size_t(1));
    QCOMPARE(openWithProvider.requests.at(0).targetUrl, imageUrl);
}

void TestKiriDocumentSession::openWithFailureEmitsToastSignal()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QVERIFY(writeTestImage(imagePath));
    const QUrl imageUrl = localUrl(imagePath);

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl) });
    FakeMediaOpenWithProvider openWithProvider;
    openWithProvider.result = KiriView::MediaOpenWithResult::Failed;
    openWithProvider.errorString = QStringLiteral("launcher failed");
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, nullptr, {},
            KiriView::TestSupport::staticImageDataDecoder(), openWithProvider.provider());
    QSignalSpy failureSpy(session.get(), &KiriDocumentSession::openWithFailed);

    session->setSourceUrl(imageUrl);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    session->openCurrentMediaWith();

    QCOMPARE(failureSpy.count(), 1);
    QCOMPARE(failureSpy.at(0).at(0).toString(), QStringLiteral("launcher failed"));
}

void TestKiriDocumentSession::twoPageSpreadLastBoundaryProjectsThroughActiveNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPage = KiriView::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("03.png"));
    imageDocumentPageCandidates.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        { KiriView::TestSupport::imageDocumentPageCandidate(firstPage),
            KiriView::TestSupport::imageDocumentPageCandidate(secondPage),
            KiriView::TestSupport::imageDocumentPageCandidate(thirdPage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr, &dataLoader,
            imageDocumentPageCandidates.provider(),
            KiriView::TestSupport::staticImageDataDecoder(
                KiriView::TestSupport::testImage(QSize(100, 200))));
    session->imageDocument()->setViewportSize(QSizeF(400.0, 300.0));

    session->setSourceUrl(archiveUrl);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    dataLoader.finishBackLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    session->imageDocument()->setTwoPageModeEnabled(true);
    session->imageDocument()->openNextPage();
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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl video = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage), directMediaNavigationCandidate(video),
            directMediaNavigationCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    imageDataLoader.finishFrontLoad(QByteArrayLiteral("first"));
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
    imageDataLoader.finishBackLoad(QByteArrayLiteral("next"));

    session->openNextActiveNavigation();
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), video);

    const std::size_t loadCountBeforeReturn = imageDataLoader.loadCount();
    session->openPreviousActiveNavigation();

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), firstImage);
    QCOMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(imageDataLoader.loadCount(), loadCountBeforeReturn);
}

void TestKiriDocumentSession::videoActiveNavigationExposesCurrentNumberAndCount()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl videoUrl = localUrl(QStringLiteral("/media/02.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);

    session->setSourceUrl(videoUrl);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());

    session->openActiveNavigationAtNumber(1);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QCOMPARE(session->sourceUrl(), imageUrl);
    QCOMPARE(session->imageDocument()->sourceUrl(), imageUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), QUrl());
}

void TestKiriDocumentSession::initialDirectImagePredecodeUsesRequestedMediaCursor()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl video = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/03.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage), directMediaNavigationCandidate(video),
            directMediaNavigationCandidate(nextImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, nullptr, &imageDataLoader);

    session->setSourceUrl(firstImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, firstImage);
    QCOMPARE(session->imageDocument()->displayedUrl(), QUrl());
    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, nextImage);
}

void TestKiriDocumentSession::directImagePredecodeDoesNotUseImageDocumentPageCandidates()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::FakeImageDocumentPageCandidateProvider imageDocumentPageCandidates;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl currentImage = localUrl(QStringLiteral("/media/current.png"));
    const QUrl directMediaNextImage = localUrl(QStringLiteral("/media/next.png"));
    const QUrl imageDocumentOnlyImage = localUrl(QStringLiteral("/media/page-only.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(currentImage),
            directMediaNavigationCandidate(directMediaNextImage) });
    imageDocumentPageCandidates.setDirectoryImages(localUrl(QStringLiteral("/media/")),
        { KiriView::TestSupport::imageDocumentPageCandidate(currentImage),
            KiriView::TestSupport::imageDocumentPageCandidate(imageDocumentOnlyImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), nullptr,
            &imageDataLoader, imageDocumentPageCandidates.provider());

    session->setSourceUrl(currentImage);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(1));
    QCOMPARE(imageDataLoader.frontLoad().url, currentImage);
    QTRY_COMPARE(imageDataLoader.loadCount(), std::size_t(2));
    QCOMPARE(imageDataLoader.backLoad().url, directMediaNextImage);
    QVERIFY(imageDataLoader.finishOldestActiveLoadForUrl(
        directMediaNextImage, QByteArrayLiteral("direct-next")));
    QVERIFY(
        imageDataLoader.finishOldestActiveLoadForUrl(currentImage, QByteArrayLiteral("current")));
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);

    QTest::qWait(250);

    QCOMPARE(imageDataLoader.loadCount(), std::size_t(2));
}

void TestKiriDocumentSession::
    staleDirectMediaNavigationCandidateCompletionCannotPublishForNewSource()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider());

    const QUrl firstClip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl secondClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl secondSibling = localUrl(QStringLiteral("/media/03.mp4"));

    session->setSourceUrl(firstClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));

    session->setSourceUrl(secondClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QCOMPARE(
        directMediaNavigationProvider.loadAt(1).parentUrl, localUrl(QStringLiteral("/media/")));
    QVERIFY(directMediaNavigationProvider.loadAt(0).canceled);
    QVERIFY(!session->activeNavigationKnown());

    directMediaNavigationProvider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(secondClip),
            directMediaNavigationCandidate(secondSibling) });

    QVERIFY(!session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 0);
    QCOMPARE(session->activeNavigationCount(), 0);

    directMediaNavigationProvider.finishLoad(1,
        { directMediaNavigationCandidate(firstClip), directMediaNavigationCandidate(secondClip) });

    QVERIFY(session->activeNavigationKnown());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 2);
}

void TestKiriDocumentSession::
    nextMediaFromVideoCanRouteToImageWithoutUsingImageDocumentPageNavigation()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl image = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(image) });
    std::unique_ptr<KiriDocumentSession> session = createSession(directMediaNavigationProvider);
    session->setSourceUrl(clip);

    QVERIFY(session->canOpenNextActiveNavigation());
    session->openNextActiveNavigation();

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

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl directoryUrl = localUrl(directory.path());
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);
    QSignalSpy progressSpy(session.get(), &KiriDocumentSession::fileDeletionInProgressChanged);

    session->setSourceUrl(directoryUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::ImageDocumentPageNavigationBoundary);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, directoryUrl);
    QVERIFY(session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 1);

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(progressSpy.count(), 2);
}

void TestKiriDocumentSession::directMediaDeletionInProgressDisablesActiveNavigationDispatch()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl firstClip = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl currentClip = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl lastClip = localUrl(QStringLiteral("/media/03.mp4"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstClip), directMediaNavigationCandidate(currentClip),
            directMediaNavigationCandidate(lastClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);

    session->setSourceUrl(currentClip);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(session->displayedFileDeletionAvailable());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(session->canOpenPreviousActiveNavigation());
    QVERIFY(session->canOpenNextActiveNavigation());

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, currentClip);
    QVERIFY(session->fileDeletionInProgress());
    QVERIFY(!session->displayedFileDeletionAvailable());
    QVERIFY(session->activeNavigationAvailable());
    QVERIFY(session->activeNavigationKnown());
    QVERIFY(!session->activeNavigationEditable());
    QCOMPARE(session->activeNavigationCurrentNumber(), 2);
    QCOMPARE(session->activeNavigationCount(), 3);
    QVERIFY(!session->canOpenPreviousActiveNavigation());
    QVERIFY(!session->canOpenNextActiveNavigation());
    QVERIFY(!session->atKnownFirstActiveNavigation());
    QVERIFY(!session->atKnownLastActiveNavigation());
    QCOMPARE(session->requestPreviousActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);
    QCOMPARE(session->requestNextActiveNavigation(),
        KiriDocumentSession::ActiveNavigationRequestResult::NoActiveNavigationRequestResult);

    session->openPreviousActiveNavigation();
    session->openNextActiveNavigation();
    session->openFirstActiveNavigation();
    session->openLastActiveNavigation();
    session->openActiveNavigationAtNumber(1);
    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(session->sourceUrl(), currentClip);
    QCOMPARE(session->videoDocument()->sourceUrl(), currentClip);
    QCOMPARE(fileOperations.operationCount(), std::size_t(1));

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QVERIFY(!session->fileDeletionInProgress());
    QCOMPARE(session->sourceUrl(), currentClip);
}

void TestKiriDocumentSession::directImageDeletionCanOpenVideoFallback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString imagePath = directory.filePath(QStringLiteral("01.png"));
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl imageUrl = localUrl(imagePath);
    const QUrl videoUrl = localUrl(directory.filePath(QStringLiteral("02.mp4")));
    directMediaNavigationProvider.setMedia(localUrl(directory.path() + QStringLiteral("/")),
        { directMediaNavigationCandidate(imageUrl), directMediaNavigationCandidate(videoUrl) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);
    session->setSourceUrl(imageUrl);
    QTRY_COMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Image);
    QTRY_COMPARE(session->imageDocument()->status(), KiriImageDocument::Status::Ready);
    QCOMPARE(session->activeNavigationBoundaryScope(),
        KiriDocumentSession::ActiveNavigationBoundaryScope::DirectMediaNavigationBoundary);

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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    KiriView::TestSupport::ManualImageDataLoader imageDataLoader;
    const QUrl firstImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl secondImage = localUrl(QStringLiteral("/media/02.png"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/media/")),
        { directMediaNavigationCandidate(firstImage),
            directMediaNavigationCandidate(secondImage) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations, &imageDataLoader);

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
    QCOMPARE(session->activeNavigationCurrentNumber(), 1);

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, firstImage);
}

void TestKiriDocumentSession::pendingDirectMediaDeletionCandidateLoadIsCanceledBySourceChange()
{
    ManualDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mp4"));
    const QUrl firstFallback = localUrl(QStringLiteral("/first/02.png"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mp4"));
    std::unique_ptr<KiriDocumentSession> session
        = createSessionWithProvider(directMediaNavigationProvider.provider(), &fileOperations);

    session->setSourceUrl(firstClip);
    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(1));
    directMediaNavigationProvider.finishLoad(0,
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(firstFallback) });

    session->deleteDisplayedFile(KiriDocumentSession::DeletionMode::MoveToTrash);

    QCOMPARE(directMediaNavigationProvider.loadCount(), std::size_t(2));
    QVERIFY(session->fileDeletionInProgress());
    QCOMPARE(fileOperations.operationCount(), std::size_t(0));

    session->setSourceUrl(secondClip);

    QCOMPARE(session->sourceUrl(), secondClip);
    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QVERIFY(!session->fileDeletionInProgress());
    QVERIFY(directMediaNavigationProvider.loadAt(1).canceled);

    directMediaNavigationProvider.deliverIgnoringCancellation(1,
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(firstFallback) });

    QCOMPARE(fileOperations.operationCount(), std::size_t(0));
    QCOMPARE(session->sourceUrl(), secondClip);
    QVERIFY(!session->fileDeletionInProgress());
}

void TestKiriDocumentSession::videoDeletionUsesOriginalUrlAndOpensMediaFallback()
{
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl clip = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/01.mp4"));
    const QUrl fallback = QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/02.jpg"));
    directMediaNavigationProvider.setMedia(
        QUrl(QStringLiteral("zip:///path/archive.zip!/chapter/")),
        { directMediaNavigationCandidate(clip), directMediaNavigationCandidate(fallback) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);
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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl clip = localUrl(QStringLiteral("/media/01.mov"));
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/media/")), { directMediaNavigationCandidate(clip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);
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
    FakeDirectMediaNavigationCandidateProvider directMediaNavigationProvider;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    const QUrl firstClip = localUrl(QStringLiteral("/first/01.mov"));
    const QUrl staleFallback = localUrl(QStringLiteral("/first/02.png"));
    const QUrl secondClip = localUrl(QStringLiteral("/second/01.mov"));
    directMediaNavigationProvider.setMedia(localUrl(QStringLiteral("/first/")),
        { directMediaNavigationCandidate(firstClip),
            directMediaNavigationCandidate(staleFallback) });
    directMediaNavigationProvider.setMedia(
        localUrl(QStringLiteral("/second/")), { directMediaNavigationCandidate(secondClip) });
    std::unique_ptr<KiriDocumentSession> session
        = createSession(directMediaNavigationProvider, &fileOperations);
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
