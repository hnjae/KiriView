// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpagecache.h"

#include "image_test_support.h"

#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using kiriview::TestSupport::localUrl;
}

class TestImageSpreadPageCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cacheNormalizesUrlKeys();
    void emptyUrlsAndSizesAreIgnored();
};

void TestImageSpreadPageCache::cacheNormalizesUrlKeys()
{
    kiriview::ImageSpreadPageCache cache;

    cache.cachePageSize(localUrl(QStringLiteral("/books/chapter/../page.png")), QSize(1200, 800));

    const std::optional<bool> cached
        = cache.cachedPageIsWide(localUrl(QStringLiteral("/books/page.png")));
    QVERIFY(cached.has_value());
    QVERIFY(*cached);
}

void TestImageSpreadPageCache::emptyUrlsAndSizesAreIgnored()
{
    kiriview::ImageSpreadPageCache cache;
    const QUrl pageUrl = localUrl(QStringLiteral("/books/page.png"));

    cache.cachePageSize(QUrl(), QSize(1200, 800));
    cache.cachePageSize(pageUrl, QSize());

    QVERIFY(!cache.cachedPageIsWide(QUrl()).has_value());
    QVERIFY(!cache.cachedPageIsWide(pageUrl).has_value());
}

QTEST_GUILESS_MAIN(TestImageSpreadPageCache)

#include "test_imagespreadpagecache.moc"
