// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "image_async_test_support.h"
#include "image_test_support.h"
#include "navigation/medianavigationmodel.h"

#include <QImage>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
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

std::unique_ptr<KiriDocumentSession> createSession(FakeMediaCandidateProvider &mediaProvider,
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr,
    KiriView::TestSupport::ManualImageDataLoader *imageDataLoader = nullptr)
{
    KiriView::DocumentSessionRuntimeDependencies dependencies;
    dependencies.mediaCandidateProvider = mediaProvider.provider();
    if (fileOperations != nullptr) {
        dependencies.fileOperationProvider
            = KiriView::TestSupport::fileOperationProviderFor(*fileOperations);
    }
    if (imageDataLoader != nullptr) {
        dependencies.imageDocumentDependencies.imageDecode
            = KiriView::TestSupport::imageDecodeDependenciesFor(
                *imageDataLoader, KiriView::TestSupport::staticImageDataDecoder());
    }
    return std::make_unique<KiriDocumentSession>(std::move(dependencies));
}
}

class TestKiriDocumentSession : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directVideoRoutesToVideoDocumentWithOriginalSource();
    void archiveAndDirectoryInputsRouteToImageDocument();
    void directImageAfterVideoRestoresImageDocument();
    void directImageMediaNavigationIncludesSiblingVideos();
    void videoNavigationKeepsStillImagePredecodeCache();
    void videoMediaNavigationExposesCurrentNumberAndCount();
    void nextMediaFromVideoCanRouteToImageWithoutUsingImageNavigation();
    void directImageDeletionCanOpenVideoFallback();
    void videoDeletionUsesOriginalUrlAndOpensMediaFallback();
    void canceledVideoDeletionKeepsCurrentVideo();
};

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
    QImage image(QSize(2, 2), QImage::Format_RGBA8888);
    image.fill(Qt::red);
    QVERIFY(image.save(imagePath, "PNG"));

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

    session->openMediaAtNumber(2);

    QCOMPARE(session->documentKind(), KiriDocumentSession::DocumentKind::Video);
    QCOMPARE(session->sourceUrl(), videoUrl);
    QCOMPARE(session->videoDocument()->sourceUrl(), videoUrl);
    QCOMPARE(session->currentMediaNumber(), 2);
    QCOMPARE(session->mediaCount(), 2);
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

QTEST_GUILESS_MAIN(TestKiriDocumentSession)

#include "test_kiridocumentsession.moc"
