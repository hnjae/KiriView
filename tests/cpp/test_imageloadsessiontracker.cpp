// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "candidate_test_support.h"
#include "decoding/imagedecoderequest.h"
#include "document/imageloadsessiontracker.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <limits>
#include <optional>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::videoCandidate;
}

class TestImageLoadSessionTracker : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startOwnsSessionIdAndFirstDisplayContext();
    void staleSessionsCannotResolveOrFinishCurrentLoad();
    void archiveResolutionUpdatesCanonicalCurrentSession();
    void archiveResolutionReportsUnsupportedOpenedCollectionVideo();
    void archiveResolutionUsesCandidateKindInsteadOfExtension();
    void emptyOpenedCollectionResolutionClaimsCurrentSessionForError();
    void predecodedLocationReplacementUpdatesCanonicalCurrentSession();
    void decodeRequestClaimClearsTheActiveSession();
    void claimCurrentClearsTheActiveSession();
    void sessionIdsStayNonZeroAfterWrap();
};

void TestImageLoadSessionTracker::startOwnsSessionIdAndFirstDisplayContext()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));

    const KiriView::ImageLoadPlan firstPlan
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(firstUrl),
            KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) });
    const KiriView::ImageLoadPlan secondPlan
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(secondUrl));

    QCOMPARE(firstPlan.session.id(), quint64(1));
    QCOMPARE(secondPlan.session.id(), quint64(2));
    QCOMPARE(firstPlan.session.firstDisplay().physicalViewportSize, QSize(320, 240));
    QCOMPARE(secondPlan.session.firstDisplay().physicalViewportSize, QSize());
    QVERIFY(!tracker.isCurrent(firstPlan.session));
    QVERIFY(tracker.isCurrent(secondPlan.session));
}

void TestImageLoadSessionTracker::staleSessionsCannotResolveOrFinishCurrentLoad()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));

    const KiriView::ImageLoadSession staleSession
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(firstUrl)).session;
    const KiriView::ImageLoadSession currentSession
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(secondUrl)).session;

    const KiriView::OpenedCollectionCandidateCompletion staleArchiveCompletion
        = tracker.completeOpenedCollectionCandidates(
            staleSession, { imageDocumentPageCandidate(firstUrl) });
    QCOMPARE(staleArchiveCompletion.action,
        KiriView::OpenedCollectionCandidateCompletionAction::Ignored);
    QVERIFY(!tracker
            .claimPredecodedImage(staleSession, KiriView::DisplayedImageLocation::fromUrl(firstUrl))
            .has_value());
    QVERIFY(!tracker.claimCurrent(staleSession).has_value());

    const KiriView::ImageDecodeRequest staleRequest
        = KiriView::ImageDecodeRequest::fromUrl(staleSession.id(), firstUrl);
    QVERIFY(!tracker.claimCurrentForDecodeRequest(staleRequest).has_value());

    const KiriView::ImageDecodeRequest currentRequest
        = KiriView::ImageDecodeRequest::fromUrl(currentSession.id(), secondUrl);
    QVERIFY(tracker.claimCurrentForDecodeRequest(currentRequest).has_value());
}

void TestImageLoadSessionTracker::archiveResolutionUpdatesCanonicalCurrentSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));

    const KiriView::ImageLoadSession session
        = tracker
              .start(KiriView::ImageLoadRequest::fromUrl(archiveUrl),
                  KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) })
              .session;
    const KiriView::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(
            session, { imageDocumentPageCandidate(imageUrl) });

    QCOMPARE(
        completion.action, KiriView::OpenedCollectionCandidateCompletionAction::StartImageDecode);
    const KiriView::ImageLoadSession &resolvedSession = completion.session;
    QCOMPARE(resolvedSession.imageUrl(), imageUrl);
    QCOMPARE(resolvedSession.kind(), KiriView::ImageDocumentPageKind::Image);
    QCOMPARE(resolvedSession.firstDisplay().physicalViewportSize, QSize(320, 240));
    const KiriView::ImageDecodeRequest request = resolvedSession.decodeRequest();
    const std::optional<KiriView::ImageLoadSession> currentSession
        = tracker.claimCurrentForDecodeRequest(request);
    QVERIFY(currentSession.has_value());
    QCOMPARE(currentSession->imageUrl(), imageUrl);
    QCOMPARE(currentSession->firstDisplay().physicalViewportSize, QSize(320, 240));
}

void TestImageLoadSessionTracker::archiveResolutionReportsUnsupportedOpenedCollectionVideo()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl videoUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.bin"));

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl)).session;
    const KiriView::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(session, { videoCandidate(videoUrl) });

    QCOMPARE(completion.action,
        KiriView::OpenedCollectionCandidateCompletionAction::
            ReportUnsupportedOpenedCollectionVideo);
    QCOMPARE(completion.session.imageUrl(), videoUrl);
    QCOMPARE(completion.session.kind(), KiriView::ImageDocumentPageKind::Video);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::archiveResolutionUsesCandidateKindInsteadOfExtension()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.mp4"));

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl)).session;
    const KiriView::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(
            session, { imageDocumentPageCandidate(imageUrl) });

    QCOMPARE(
        completion.action, KiriView::OpenedCollectionCandidateCompletionAction::StartImageDecode);
    QCOMPARE(completion.session.imageUrl(), imageUrl);
    QCOMPARE(completion.session.kind(), KiriView::ImageDocumentPageKind::Image);
}

void TestImageLoadSessionTracker::emptyOpenedCollectionResolutionClaimsCurrentSessionForError()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl)).session;

    const KiriView::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(session, {});

    QCOMPARE(completion.action,
        KiriView::OpenedCollectionCandidateCompletionAction::ReportEmptyOpenedCollection);
    QCOMPARE(completion.session.imageUrl(), archiveUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::predecodedLocationReplacementUpdatesCanonicalCurrentSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(imageUrl)).session;
    const std::optional<KiriView::ImageLoadSession> replacedSession = tracker.claimPredecodedImage(
        session,
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(imageUrl, *archiveCollection));

    QVERIFY(replacedSession.has_value());
    QCOMPARE(replacedSession->imageUrl(), imageUrl);
    QCOMPARE(
        replacedSession->location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::decodeRequestClaimClearsTheActiveSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(imageUrl)).session;
    const KiriView::ImageDecodeRequest request = session.decodeRequest();

    const std::optional<KiriView::ImageLoadSession> claimedSession
        = tracker.claimCurrentForDecodeRequest(request);

    QVERIFY(claimedSession.has_value());
    QCOMPARE(claimedSession->imageUrl(), imageUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::claimCurrentClearsTheActiveSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(imageUrl)).session;

    const std::optional<KiriView::ImageLoadSession> takenSession = tracker.claimCurrent(session);

    QVERIFY(takenSession.has_value());
    QCOMPARE(takenSession->imageUrl(), imageUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::sessionIdsStayNonZeroAfterWrap()
{
    KiriView::ImageLoadSessionTracker tracker(std::numeric_limits<quint64>::max());

    const KiriView::ImageLoadPlan firstPlan
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(localUrl(QStringLiteral("/1.png"))));
    const KiriView::ImageLoadPlan secondPlan
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(localUrl(QStringLiteral("/2.png"))));

    QCOMPARE(firstPlan.session.id(), quint64(1));
    QCOMPARE(secondPlan.session.id(), quint64(2));
}

QTEST_GUILESS_MAIN(TestImageLoadSessionTracker)

#include "test_imageloadsessiontracker.moc"
