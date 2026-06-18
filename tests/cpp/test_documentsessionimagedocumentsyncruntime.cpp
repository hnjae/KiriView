// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionimagedocumentsyncruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

class TestDocumentSessionImageDocumentSyncRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ignoresRoutingAndInactiveDocumentKind();
    void confirmsDirectImageCursorBeforeRefreshingNavigation();
    void mirrorsDeletionProgressWhenImageDocumentOwnsSourceScope();
    void publishesImagePageNavigationWhenTheLeafNavigationChanges();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

struct ImageSyncFixture {
    enum class Event {
        ConfirmDirectImageCursor,
        RestoreDirectImageCursorAfterFailure,
        SetSourceIdentity,
        SetFileDeletionInProgress,
        RefreshNavigation,
        CacheDisplayedPredecode,
        PublishImagePages,
        Publish,
    };

    std::vector<Event> events;
    QUrl confirmedCursorUrl;
    bool confirmedCursorChanged = true;
    bool restoredCursorChanged = true;
    QUrl sourceIdentity;
    bool fileDeletionInProgress = false;
    kiriview::DocumentSessionImageDocumentSyncRuntime runtime {
        kiriview::DocumentSessionImageDocumentSyncRuntimePorts {
            [this](const QUrl &url) {
                events.push_back(Event::ConfirmDirectImageCursor);
                confirmedCursorUrl = url;
                return confirmedCursorChanged;
            },
            [this]() {
                events.push_back(Event::RestoreDirectImageCursorAfterFailure);
                return restoredCursorChanged;
            },
            [this](const QUrl &url) {
                events.push_back(Event::SetSourceIdentity);
                sourceIdentity = url;
            },
            [this](bool inProgress) {
                events.push_back(Event::SetFileDeletionInProgress);
                fileDeletionInProgress = inProgress;
            },
            [this]() { events.push_back(Event::RefreshNavigation); },
            [this]() { events.push_back(Event::CacheDisplayedPredecode); },
            [this]() { events.push_back(Event::PublishImagePages); },
            [this]() { events.push_back(Event::Publish); },
        }
    };
};

kiriview::DocumentSessionImageDocumentSyncRuntimeInput activeInput(const QUrl &url)
{
    kiriview::DocumentSessionImageDocumentSyncRuntimeInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.directMediaNavigationActive = true;
    input.directMediaNavigationKnown = true;
    input.image.sourceUrl = url;
    input.image.displayedUrl = url;
    input.image.ordinaryDirectMediaScopeActive = true;
    return input;
}
}

void TestDocumentSessionImageDocumentSyncRuntime::ignoresRoutingAndInactiveDocumentKind()
{
    ImageSyncFixture fixture;
    kiriview::DocumentSessionImageDocumentSyncRuntimeInput input
        = activeInput(localUrl(QStringLiteral("/media/01.png")));
    input.routingSource = true;

    fixture.runtime.sync(input);

    QVERIFY(fixture.events.empty());

    input.routingSource = false;
    input.documentKind = kiriview::DocumentSessionKind::Video;

    fixture.runtime.sync(input);

    QVERIFY(fixture.events.empty());
}

void TestDocumentSessionImageDocumentSyncRuntime::
    confirmsDirectImageCursorBeforeRefreshingNavigation()
{
    ImageSyncFixture fixture;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    kiriview::DocumentSessionImageDocumentSyncRuntimeInput input = activeInput(imageUrl);
    input.directImageLoadMayUseImageDocumentSourceScope = true;
    input.directMediaCursor.pendingUrl = imageUrl;

    fixture.runtime.sync(input);

    QCOMPARE(fixture.confirmedCursorUrl, imageUrl);
    QCOMPARE(fixture.sourceIdentity, imageUrl);
    QCOMPARE(fixture.events,
        (std::vector<ImageSyncFixture::Event> {
            ImageSyncFixture::Event::ConfirmDirectImageCursor,
            ImageSyncFixture::Event::SetSourceIdentity,
            ImageSyncFixture::Event::RefreshNavigation,
            ImageSyncFixture::Event::Publish,
        }));
}

void TestDocumentSessionImageDocumentSyncRuntime::
    mirrorsDeletionProgressWhenImageDocumentOwnsSourceScope()
{
    ImageSyncFixture fixture;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    kiriview::DocumentSessionImageDocumentSyncRuntimeInput input = activeInput(imageUrl);
    input.directImageLoadMayUseImageDocumentSourceScope = false;
    input.directMediaNavigationActive = false;
    input.image.fileDeletionInProgress = true;

    fixture.runtime.sync(input);

    QVERIFY(fixture.fileDeletionInProgress);
    QCOMPARE(fixture.events,
        (std::vector<ImageSyncFixture::Event> {
            ImageSyncFixture::Event::ConfirmDirectImageCursor,
            ImageSyncFixture::Event::SetSourceIdentity,
            ImageSyncFixture::Event::SetFileDeletionInProgress,
            ImageSyncFixture::Event::RefreshNavigation,
            ImageSyncFixture::Event::Publish,
        }));
}

void TestDocumentSessionImageDocumentSyncRuntime::
    publishesImagePageNavigationWhenTheLeafNavigationChanges()
{
    ImageSyncFixture fixture;
    fixture.confirmedCursorChanged = false;
    const QUrl imageUrl = localUrl(QStringLiteral("/media/01.png"));
    kiriview::DocumentSessionImageDocumentSyncRuntimeInput input = activeInput(imageUrl);
    input.directImageLoadMayUseImageDocumentSourceScope = true;
    input.previousPageNavigation.known = false;
    input.image.pageNavigation.known = true;
    input.image.pageNavigation.currentNumber = 2;
    input.image.pageNavigation.count = 3;

    fixture.runtime.sync(input);

    QCOMPARE(fixture.events,
        (std::vector<ImageSyncFixture::Event> {
            ImageSyncFixture::Event::ConfirmDirectImageCursor,
            ImageSyncFixture::Event::SetSourceIdentity,
            ImageSyncFixture::Event::CacheDisplayedPredecode,
            ImageSyncFixture::Event::PublishImagePages,
        }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionImageDocumentSyncRuntime)
#include "test_documentsessionimagedocumentsyncruntime.moc"
