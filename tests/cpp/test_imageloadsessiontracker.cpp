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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::videoCandidate;
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
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));

    const kiriview::ImageLoadPlan firstPlan
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(firstUrl),
            kiriview::ImageFirstDisplayDecodeContext { QSize(320, 240) });
    const kiriview::ImageLoadPlan secondPlan
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(secondUrl));

    QCOMPARE(firstPlan.session.id(), quint64(1));
    QCOMPARE(secondPlan.session.id(), quint64(2));
    QCOMPARE(firstPlan.session.firstDisplay().physicalViewportSize, QSize(320, 240));
    QCOMPARE(secondPlan.session.firstDisplay().physicalViewportSize, QSize());
    QVERIFY(!tracker.isCurrent(firstPlan.session));
    QVERIFY(tracker.isCurrent(secondPlan.session));
}

void TestImageLoadSessionTracker::staleSessionsCannotResolveOrFinishCurrentLoad()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));

    const kiriview::ImageLoadSession staleSession
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(firstUrl)).session;
    const kiriview::ImageLoadSession currentSession
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(secondUrl)).session;

    const kiriview::OpenedCollectionCandidateCompletion staleArchiveCompletion
        = tracker.completeOpenedCollectionCandidates(
            staleSession, { imageDocumentPageCandidate(firstUrl) });
    QCOMPARE(staleArchiveCompletion.action,
        kiriview::OpenedCollectionCandidateCompletionAction::Ignored);
    QVERIFY(!tracker
            .claimPredecodedImage(staleSession, kiriview::DisplayedImageLocation::fromUrl(firstUrl))
            .has_value());
    QVERIFY(!tracker.claimCurrent(staleSession).has_value());

    const kiriview::ImageDecodeRequest staleRequest
        = kiriview::ImageDecodeRequest::fromUrl(staleSession.id(), firstUrl);
    QVERIFY(!tracker.claimCurrentForDecodeRequest(staleRequest).has_value());

    const kiriview::ImageDecodeRequest currentRequest
        = kiriview::ImageDecodeRequest::fromUrl(currentSession.id(), secondUrl);
    QVERIFY(tracker.claimCurrentForDecodeRequest(currentRequest).has_value());
}

void TestImageLoadSessionTracker::archiveResolutionUpdatesCanonicalCurrentSession()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));

    const kiriview::ImageLoadSession session
        = tracker
              .start(kiriview::ImageLoadRequest::fromUrl(archiveUrl),
                  kiriview::ImageFirstDisplayDecodeContext { QSize(320, 240) })
              .session;
    const kiriview::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(
            session, { imageDocumentPageCandidate(imageUrl) });

    QCOMPARE(
        completion.action, kiriview::OpenedCollectionCandidateCompletionAction::StartImageDecode);
    const kiriview::ImageLoadSession &resolvedSession = completion.session;
    QCOMPARE(resolvedSession.imageUrl(), imageUrl);
    QCOMPARE(resolvedSession.kind(), kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(resolvedSession.firstDisplay().physicalViewportSize, QSize(320, 240));
    const kiriview::ImageDecodeRequest request = resolvedSession.decodeRequest();
    const std::optional<kiriview::ImageLoadSession> currentSession
        = tracker.claimCurrentForDecodeRequest(request);
    QVERIFY(currentSession.has_value());
    QCOMPARE(currentSession->imageUrl(), imageUrl);
    QCOMPARE(currentSession->firstDisplay().physicalViewportSize, QSize(320, 240));
}

void TestImageLoadSessionTracker::archiveResolutionReportsUnsupportedOpenedCollectionVideo()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl videoUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.bin"));

    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl)).session;
    const kiriview::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(session, { videoCandidate(videoUrl) });

    QCOMPARE(completion.action,
        kiriview::OpenedCollectionCandidateCompletionAction::
            ReportUnsupportedOpenedCollectionVideo);
    QCOMPARE(completion.session.imageUrl(), videoUrl);
    QCOMPARE(completion.session.kind(), kiriview::ImageDocumentPageKind::Video);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::archiveResolutionUsesCandidateKindInsteadOfExtension()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.mp4"));

    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl)).session;
    const kiriview::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(
            session, { imageDocumentPageCandidate(imageUrl) });

    QCOMPARE(
        completion.action, kiriview::OpenedCollectionCandidateCompletionAction::StartImageDecode);
    QCOMPARE(completion.session.imageUrl(), imageUrl);
    QCOMPARE(completion.session.kind(), kiriview::ImageDocumentPageKind::Image);
}

void TestImageLoadSessionTracker::emptyOpenedCollectionResolutionClaimsCurrentSessionForError()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));

    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(archiveUrl)).session;

    const kiriview::OpenedCollectionCandidateCompletion completion
        = tracker.completeOpenedCollectionCandidates(session, {});

    QCOMPARE(completion.action,
        kiriview::OpenedCollectionCandidateCompletionAction::ReportEmptyOpenedCollection);
    QCOMPARE(completion.session.imageUrl(), archiveUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::predecodedLocationReplacementUpdatesCanonicalCurrentSession()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(imageUrl)).session;
    const std::optional<kiriview::ImageLoadSession> replacedSession = tracker.claimPredecodedImage(
        session,
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(imageUrl, *archiveCollection));

    QVERIFY(replacedSession.has_value());
    QCOMPARE(replacedSession->imageUrl(), imageUrl);
    QCOMPARE(
        replacedSession->location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::decodeRequestClaimClearsTheActiveSession()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(imageUrl)).session;
    const kiriview::ImageDecodeRequest request = session.decodeRequest();

    const std::optional<kiriview::ImageLoadSession> claimedSession
        = tracker.claimCurrentForDecodeRequest(request);

    QVERIFY(claimedSession.has_value());
    QCOMPARE(claimedSession->imageUrl(), imageUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::claimCurrentClearsTheActiveSession()
{
    kiriview::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const kiriview::ImageLoadSession session
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(imageUrl)).session;

    const std::optional<kiriview::ImageLoadSession> takenSession = tracker.claimCurrent(session);

    QVERIFY(takenSession.has_value());
    QCOMPARE(takenSession->imageUrl(), imageUrl);
    QVERIFY(!tracker.isCurrent(session));
}

void TestImageLoadSessionTracker::sessionIdsStayNonZeroAfterWrap()
{
    kiriview::ImageLoadSessionTracker tracker(std::numeric_limits<quint64>::max());

    const kiriview::ImageLoadPlan firstPlan
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(localUrl(QStringLiteral("/1.png"))));
    const kiriview::ImageLoadPlan secondPlan
        = tracker.start(kiriview::ImageLoadRequest::fromUrl(localUrl(QStringLiteral("/2.png"))));

    QCOMPARE(firstPlan.session.id(), quint64(1));
    QCOMPARE(secondPlan.session.id(), quint64(2));
}

QTEST_GUILESS_MAIN(TestImageLoadSessionTracker)

#include "test_imageloadsessiontracker.moc"
