// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionvideodocumentsyncruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

class TestDocumentSessionVideoDocumentSyncRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ignoresInactiveDocumentKind();
    void emptyVideoSourceClearsSessionDirectMedia();
    void directVideoSourceCommitsCursorAndRefreshesWhenScopeChanged();
    void unchangedDirectVideoCursorSkipsRefresh();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

struct VideoSyncFixture {
    enum class Event {
        ClearCursor,
        SetSourceIdentity,
        SetDocumentKind,
        ClearNavigation,
        SetDirectVideoCursor,
        RefreshNavigation,
        Publish,
        ActiveZoom,
    };

    std::vector<Event> events;
    QUrl sourceIdentity;
    kiriview::DocumentSessionKind documentKind = kiriview::DocumentSessionKind::Video;
    QUrl directVideoCursorUrl;
    bool directVideoCursorChanged = true;
    kiriview::DocumentSessionVideoDocumentSyncRuntime runtime {
        kiriview::DocumentSessionVideoDocumentSyncRuntimePorts {
            [this]() { events.push_back(Event::ClearCursor); },
            [this](const QUrl &url) {
                events.push_back(Event::SetSourceIdentity);
                sourceIdentity = url;
            },
            [this](kiriview::DocumentSessionKind kind) {
                events.push_back(Event::SetDocumentKind);
                documentKind = kind;
            },
            [this]() { events.push_back(Event::ClearNavigation); },
            [this](const QUrl &url) {
                events.push_back(Event::SetDirectVideoCursor);
                directVideoCursorUrl = url;
                return directVideoCursorChanged;
            },
            [this]() { events.push_back(Event::RefreshNavigation); },
            [this]() { events.push_back(Event::Publish); },
            [this]() { events.push_back(Event::ActiveZoom); },
        }
    };
};

kiriview::DocumentSessionPublicVideoLeafSnapshot videoSnapshot(const QUrl &url)
{
    kiriview::DocumentSessionPublicVideoLeafSnapshot snapshot;
    snapshot.sourceUrl = url;
    return snapshot;
}
}

void TestDocumentSessionVideoDocumentSyncRuntime::ignoresInactiveDocumentKind()
{
    VideoSyncFixture fixture;

    fixture.runtime.sync(kiriview::DocumentSessionKind::Image,
        videoSnapshot(localUrl(QStringLiteral("/media/clip.mkv"))));

    QVERIFY(fixture.events.empty());
}

void TestDocumentSessionVideoDocumentSyncRuntime::emptyVideoSourceClearsSessionDirectMedia()
{
    VideoSyncFixture fixture;

    fixture.runtime.sync(kiriview::DocumentSessionKind::Video, videoSnapshot(QUrl()));

    QCOMPARE(fixture.documentKind, kiriview::DocumentSessionKind::Empty);
    QVERIFY(fixture.sourceIdentity.isEmpty());
    QCOMPARE(fixture.events,
        (std::vector<VideoSyncFixture::Event> { VideoSyncFixture::Event::ClearCursor,
            VideoSyncFixture::Event::SetSourceIdentity, VideoSyncFixture::Event::SetDocumentKind,
            VideoSyncFixture::Event::ClearNavigation, VideoSyncFixture::Event::Publish,
            VideoSyncFixture::Event::ActiveZoom }));
}

void TestDocumentSessionVideoDocumentSyncRuntime::
    directVideoSourceCommitsCursorAndRefreshesWhenScopeChanged()
{
    VideoSyncFixture fixture;
    const QUrl clipUrl = localUrl(QStringLiteral("/media/clip.mkv"));

    fixture.runtime.sync(kiriview::DocumentSessionKind::Video, videoSnapshot(clipUrl));

    QCOMPARE(fixture.directVideoCursorUrl, clipUrl);
    QCOMPARE(fixture.sourceIdentity, clipUrl);
    QCOMPARE(fixture.events,
        (std::vector<VideoSyncFixture::Event> { VideoSyncFixture::Event::SetDirectVideoCursor,
            VideoSyncFixture::Event::SetSourceIdentity, VideoSyncFixture::Event::RefreshNavigation,
            VideoSyncFixture::Event::Publish, VideoSyncFixture::Event::ActiveZoom }));
}

void TestDocumentSessionVideoDocumentSyncRuntime::unchangedDirectVideoCursorSkipsRefresh()
{
    VideoSyncFixture fixture;
    fixture.directVideoCursorChanged = false;
    const QUrl clipUrl = localUrl(QStringLiteral("/media/clip.mkv"));

    fixture.runtime.sync(kiriview::DocumentSessionKind::Video, videoSnapshot(clipUrl));

    QCOMPARE(fixture.directVideoCursorUrl, clipUrl);
    QCOMPARE(fixture.sourceIdentity, clipUrl);
    QCOMPARE(fixture.events,
        (std::vector<VideoSyncFixture::Event> { VideoSyncFixture::Event::SetDirectVideoCursor,
            VideoSyncFixture::Event::SetSourceIdentity, VideoSyncFixture::Event::Publish,
            VideoSyncFixture::Event::ActiveZoom }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionVideoDocumentSyncRuntime)
#include "test_documentsessionvideodocumentsyncruntime.moc"
