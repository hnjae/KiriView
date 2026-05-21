// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiridocumentsession.h"

#include "facade/kiriimagedocument.h"
#include "facade/kirivideodocument.h"
#include "image_async_test_support.h"
#include "navigation/medianavigationmodel.h"

#include <QObject>
#include <QSignalSpy>
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
    KiriView::TestSupport::ManualFileOperationProvider *fileOperations = nullptr)
{
    KiriView::DocumentSessionRuntimeDependencies dependencies;
    dependencies.mediaCandidateProvider = mediaProvider.provider();
    if (fileOperations != nullptr) {
        dependencies.fileOperationProvider
            = KiriView::TestSupport::fileOperationProviderFor(*fileOperations);
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
    void nextMediaFromVideoCanRouteToImageWithoutUsingImageNavigation();
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
