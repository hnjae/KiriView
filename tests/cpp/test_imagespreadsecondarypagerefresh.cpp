// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadsecondarypagerefresh.h"

#include "image_test_support.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <vector>

namespace {
using kiriview::TestSupport::localUrl;

kiriview::ImageDocumentPageNavigationSnapshot navigationSnapshot(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return kiriview::ImageDocumentPageNavigationSnapshot {
        kiriview::PageNavigationState {
            urls,
            currentPageNumber - 1,
        },
    };
}

kiriview::ImageSpreadSecondaryPageRefreshRequest refreshRequest(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return kiriview::ImageSpreadSecondaryPageRefreshRequest {
        true,
        false,
        false,
        {},
        navigationSnapshot(urls, currentPageNumber),
    };
}
}

class TestImageSpreadSecondaryPageRefresh : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void plansNextPageLoadFromNavigationSnapshot();
    void keepsCurrentSecondaryWhenItAlreadyMatchesNextPage();
    void wideCachedNextPageFallsBackToPrimaryOnly();
    void videoNextPageFallsBackToPrimaryOnly();
    void previousNavigationUsesCachedPreviousPageWidth();
    void previousNavigationTreatsVideoAsSinglePage();
    void transitionPlanningUsesNavigationContext();
    void activeNavigationSnapshotUsesVisibleSpreadBoundary();
    void primarySelectionMatchingNormalizesDisplayedUrl();
    void pageWidthCacheNormalizesUrlKeys();
};

void TestImageSpreadSecondaryPageRefresh::plansNextPageLoadFromNavigationSnapshot()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    const kiriview::ImageSpreadSecondaryPageRefreshResult result
        = refresh.planRefresh(refreshRequest(urls, 2));

    QCOMPARE(result.action, kiriview::ImageSpreadSecondaryPageRefreshAction::LoadTarget);
    QCOMPARE(result.targetUrl, urls.at(2));
}

void TestImageSpreadSecondaryPageRefresh::keepsCurrentSecondaryWhenItAlreadyMatchesNextPage()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;
    kiriview::ImageSpreadSecondaryPageRefreshRequest request = refreshRequest(urls, 2);
    request.secondaryPageVisible = true;
    request.currentSecondaryUrl = urls.at(2);

    const kiriview::ImageSpreadSecondaryPageRefreshResult result = refresh.planRefresh(request);

    QCOMPARE(result.action, kiriview::ImageSpreadSecondaryPageRefreshAction::KeepCurrentSecondary);
    QVERIFY(result.targetUrl.isEmpty());
}

void TestImageSpreadSecondaryPageRefresh::wideCachedNextPageFallsBackToPrimaryOnly()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;
    refresh.cachePageSize(urls.at(2), QSize(1200, 800));

    const kiriview::ImageSpreadSecondaryPageRefreshResult result
        = refresh.planRefresh(refreshRequest(urls, 2));

    QCOMPARE(result.action, kiriview::ImageSpreadSecondaryPageRefreshAction::PrimaryOnly);
    QVERIFY(result.targetUrl.isEmpty());
}

void TestImageSpreadSecondaryPageRefresh::videoNextPageFallsBackToPrimaryOnly()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.mp4")),
        localUrl(QStringLiteral("/books/004.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    const kiriview::ImageSpreadSecondaryPageRefreshResult result
        = refresh.planRefresh(refreshRequest(urls, 2));

    QCOMPARE(result.action, kiriview::ImageSpreadSecondaryPageRefreshAction::PrimaryOnly);
    QVERIFY(result.targetUrl.isEmpty());
}

void TestImageSpreadSecondaryPageRefresh::previousNavigationUsesCachedPreviousPageWidth()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
        localUrl(QStringLiteral("/books/005.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;
    kiriview::ImageSpreadPageNavigationContext context {
        true,
        true,
        navigationSnapshot(urls, 5),
    };

    kiriview::ImageSpreadPageNavigationTarget target
        = refresh.pageNavigationTarget(kiriview::NavigationDirection::Previous, context);
    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 3);

    refresh.cachePageSize(urls.at(3), QSize(1200, 800));
    target = refresh.pageNavigationTarget(kiriview::NavigationDirection::Previous, context);
    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

void TestImageSpreadSecondaryPageRefresh::previousNavigationTreatsVideoAsSinglePage()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.mp4")),
        localUrl(QStringLiteral("/books/005.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;
    const kiriview::ImageSpreadPageNavigationContext context {
        true,
        true,
        navigationSnapshot(urls, 5),
    };

    const kiriview::ImageSpreadPageNavigationTarget target
        = refresh.pageNavigationTarget(kiriview::NavigationDirection::Previous, context);

    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

void TestImageSpreadSecondaryPageRefresh::transitionPlanningUsesNavigationContext()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
    };
    const kiriview::ImageSpreadPageNavigationContext activeContext {
        true,
        false,
        navigationSnapshot(urls, 2),
    };
    const kiriview::ImageSpreadPageNavigationContext inactiveContext {
        false,
        false,
        navigationSnapshot(urls, 2),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    QCOMPARE(refresh.currentLastPageNumber(activeContext), 2);
    QCOMPARE(refresh.relativePageNavigationTarget(1, activeContext), 3);
    QVERIFY(refresh.shouldBeginNavigationTransition(3, activeContext));
    QVERIFY(!refresh.shouldBeginNavigationTransition(3, inactiveContext));
}

void TestImageSpreadSecondaryPageRefresh::activeNavigationSnapshotUsesVisibleSpreadBoundary()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
    };
    const kiriview::ImageSpreadPageNavigationContext context {
        true,
        true,
        navigationSnapshot(urls, 2),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    const kiriview::ImageDocumentPageActiveNavigationSnapshot snapshot
        = refresh.activeNavigationSnapshot(context);

    QVERIFY(snapshot.known);
    QVERIFY(snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 3);
}

void TestImageSpreadSecondaryPageRefresh::primarySelectionMatchingNormalizesDisplayedUrl()
{
    const std::vector<QUrl> urls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/chapter/../002.png")),
    };
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    QVERIFY(refresh.primarySelectionMatchesDisplayed(
        navigationSnapshot(urls, 2), localUrl(QStringLiteral("/books/002.png"))));
    QVERIFY(!refresh.primarySelectionMatchesDisplayed(
        navigationSnapshot(urls, 2), localUrl(QStringLiteral("/books/003.png"))));
    QVERIFY(refresh.primarySelectionMatchesDisplayed(navigationSnapshot(urls, 0), QUrl()));
}

void TestImageSpreadSecondaryPageRefresh::pageWidthCacheNormalizesUrlKeys()
{
    kiriview::ImageSpreadSecondaryPageRefresh refresh;

    refresh.cachePageSize(localUrl(QStringLiteral("/books/chapter/../page.png")), QSize(1200, 800));

    const std::optional<bool> cached
        = refresh.cachedPageIsWide(localUrl(QStringLiteral("/books/page.png")));
    QVERIFY(cached.has_value());
    QVERIFY(*cached);
}

QTEST_GUILESS_MAIN(TestImageSpreadSecondaryPageRefresh)

#include "test_imagespreadsecondarypagerefresh.moc"
