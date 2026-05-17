// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagedecoderequest.h"
#include "imageloadsessiontracker.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;
}

class TestImageLoadSessionTracker : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startOwnsSessionIdAndFirstDisplayContext();
    void staleSessionsCannotResolveOrFinishCurrentLoad();
    void archiveResolutionUpdatesCanonicalCurrentSession();
    void predecodedLocationReplacementUpdatesCanonicalCurrentSession();
    void takeCurrentClearsTheActiveSession();
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

    QCOMPARE(firstPlan.session.id, quint64(1));
    QCOMPARE(secondPlan.session.id, quint64(2));
    QCOMPARE(tracker.firstDisplayContext().physicalViewportSize, QSize());
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

    QVERIFY(!tracker.resolveCurrentArchiveImage(staleSession, firstUrl).has_value());
    QVERIFY(!tracker
            .replaceCurrentLocation(
                staleSession, KiriView::DisplayedImageLocation::fromUrl(firstUrl))
            .has_value());
    QVERIFY(!tracker.takeCurrent(staleSession).has_value());

    const KiriView::ImageDecodeRequest staleRequest
        = KiriView::ImageDecodeRequest::fromUrl(staleSession.id, firstUrl);
    QVERIFY(!tracker.currentForDecodeRequest(staleRequest).has_value());

    const KiriView::ImageDecodeRequest currentRequest
        = KiriView::ImageDecodeRequest::fromUrl(currentSession.id, secondUrl);
    QVERIFY(tracker.currentForDecodeRequest(currentRequest).has_value());
}

void TestImageLoadSessionTracker::archiveResolutionUpdatesCanonicalCurrentSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(archiveUrl)).session;
    const std::optional<KiriView::ImageLoadSession> resolvedSession
        = tracker.resolveCurrentArchiveImage(session, imageUrl);

    QVERIFY(resolvedSession.has_value());
    QCOMPARE(resolvedSession->location.imageUrl(), imageUrl);
    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromUrl(session.id, imageUrl);
    const std::optional<KiriView::ImageLoadSession> currentSession
        = tracker.currentForDecodeRequest(request);
    QVERIFY(currentSession.has_value());
    QCOMPARE(currentSession->location.imageUrl(), imageUrl);
}

void TestImageLoadSessionTracker::predecodedLocationReplacementUpdatesCanonicalCurrentSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(imageUrl)).session;
    const std::optional<KiriView::ImageLoadSession> replacedSession
        = tracker.replaceCurrentLocation(session,
            KiriView::DisplayedImageLocation::fromArchiveDocument(imageUrl, *archiveDocument));

    QVERIFY(replacedSession.has_value());
    QCOMPARE(replacedSession->location.imageUrl(), imageUrl);
    QCOMPARE(replacedSession->location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromLocation(session.id, replacedSession->location);
    const std::optional<KiriView::ImageLoadSession> currentSession
        = tracker.currentForDecodeRequest(request);
    QVERIFY(currentSession.has_value());
    QCOMPARE(currentSession->location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadSessionTracker::takeCurrentClearsTheActiveSession()
{
    KiriView::ImageLoadSessionTracker tracker;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const KiriView::ImageLoadSession session
        = tracker.start(KiriView::ImageLoadRequest::fromUrl(imageUrl)).session;

    const std::optional<KiriView::ImageLoadSession> takenSession = tracker.takeCurrent(session);

    QVERIFY(takenSession.has_value());
    QCOMPARE(takenSession->location.imageUrl(), imageUrl);
    QVERIFY(!tracker.isCurrent(session));
}

QTEST_GUILESS_MAIN(TestImageLoadSessionTracker)

#include "test_imageloadsessiontracker.moc"
