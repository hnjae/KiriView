// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadsecondarypagerefresh.h"

#include "image_test_support.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;

KiriView::ImagePageNavigationSnapshot navigationSnapshot(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return KiriView::ImagePageNavigationSnapshot {
        KiriView::PageNavigationState {
            urls,
            currentPageNumber - 1,
        },
    };
}

KiriView::ImageSpreadSecondaryPageRefreshRequest refreshRequest(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return KiriView::ImageSpreadSecondaryPageRefreshRequest {
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
    KiriView::ImageSpreadSecondaryPageRefresh refresh;

    const KiriView::ImageSpreadSecondaryPageRefreshResult result
        = refresh.planRefresh(refreshRequest(urls, 2));

    QCOMPARE(result.action, KiriView::ImageSpreadSecondaryPageRefreshAction::LoadTarget);
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
    KiriView::ImageSpreadSecondaryPageRefresh refresh;
    KiriView::ImageSpreadSecondaryPageRefreshRequest request = refreshRequest(urls, 2);
    request.secondaryPageVisible = true;
    request.currentSecondaryUrl = urls.at(2);

    const KiriView::ImageSpreadSecondaryPageRefreshResult result = refresh.planRefresh(request);

    QCOMPARE(result.action, KiriView::ImageSpreadSecondaryPageRefreshAction::KeepCurrentSecondary);
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
    KiriView::ImageSpreadSecondaryPageRefresh refresh;
    refresh.cachePageSize(urls.at(2), QSize(1200, 800));

    const KiriView::ImageSpreadSecondaryPageRefreshResult result
        = refresh.planRefresh(refreshRequest(urls, 2));

    QCOMPARE(result.action, KiriView::ImageSpreadSecondaryPageRefreshAction::PrimaryOnly);
    QVERIFY(result.targetUrl.isEmpty());
}

void TestImageSpreadSecondaryPageRefresh::pageWidthCacheNormalizesUrlKeys()
{
    KiriView::ImageSpreadSecondaryPageRefresh refresh;

    refresh.cachePageSize(localUrl(QStringLiteral("/books/chapter/../page.png")), QSize(1200, 800));

    const std::optional<bool> cached
        = refresh.cachedPageIsWide(localUrl(QStringLiteral("/books/page.png")));
    QVERIFY(cached.has_value());
    QVERIFY(*cached);
}

QTEST_GUILESS_MAIN(TestImageSpreadSecondaryPageRefresh)

#include "test_imagespreadsecondarypagerefresh.moc"
