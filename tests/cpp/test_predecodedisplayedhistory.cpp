// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "location/imageurl.h"
#include "predecode/predecodedisplayedhistory.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestPredecodeDisplayedHistory : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void currentUrlsAreNormalizedAndDeduplicated();
    void previousCurrentUrlsMoveToRecentHistory();
    void redisplayedUrlsAreRemovedFromRecentHistory();
    void recentHistoryKeepsOnlyMostRecentFourUrls();
    void clearRemovesCurrentAndRecentHistory();
};

void TestPredecodeDisplayedHistory::currentUrlsAreNormalizedAndDeduplicated()
{
    kiriview::PredecodeDisplayedHistory history;
    const QUrl firstUrl = kiriview::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = kiriview::TestSupport::indexedImageUrl(2);

    history.setDisplayedUrls({ firstUrl, QUrl(), firstUrl, secondUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(2));
    QCOMPARE(history.currentUrls().at(0), kiriview::normalizedImageUrl(firstUrl));
    QCOMPARE(history.currentUrls().at(1), kiriview::normalizedImageUrl(secondUrl));
    QVERIFY(history.recentUrls().empty());
}

void TestPredecodeDisplayedHistory::previousCurrentUrlsMoveToRecentHistory()
{
    kiriview::PredecodeDisplayedHistory history;
    const QUrl firstUrl = kiriview::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = kiriview::TestSupport::indexedImageUrl(2);
    const QUrl thirdUrl = kiriview::TestSupport::indexedImageUrl(3);

    history.setDisplayedUrls({ firstUrl, secondUrl });
    history.setDisplayedUrls({ thirdUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), kiriview::normalizedImageUrl(thirdUrl));
    QCOMPARE(history.recentUrls().size(), std::size_t(2));
    QCOMPARE(history.recentUrls().at(0), kiriview::normalizedImageUrl(secondUrl));
    QCOMPARE(history.recentUrls().at(1), kiriview::normalizedImageUrl(firstUrl));
    QVERIFY(history.currentContains(kiriview::normalizedImageUrl(thirdUrl)));
    QVERIFY(history.recentContains(kiriview::normalizedImageUrl(firstUrl)));
    QCOMPARE(history.currentPriority(kiriview::normalizedImageUrl(thirdUrl)), std::size_t(0));
    QCOMPARE(history.recentPriority(kiriview::normalizedImageUrl(firstUrl)), std::size_t(1));
}

void TestPredecodeDisplayedHistory::redisplayedUrlsAreRemovedFromRecentHistory()
{
    kiriview::PredecodeDisplayedHistory history;
    const QUrl firstUrl = kiriview::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = kiriview::TestSupport::indexedImageUrl(2);

    history.setDisplayedUrls({ firstUrl });
    history.setDisplayedUrls({ secondUrl });
    history.setDisplayedUrls({ firstUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), kiriview::normalizedImageUrl(firstUrl));
    QCOMPARE(history.recentUrls().size(), std::size_t(1));
    QCOMPARE(history.recentUrls().front(), kiriview::normalizedImageUrl(secondUrl));
    QVERIFY(history.retainedContains(kiriview::normalizedImageUrl(firstUrl)));
    QVERIFY(history.retainedContains(kiriview::normalizedImageUrl(secondUrl)));
}

void TestPredecodeDisplayedHistory::recentHistoryKeepsOnlyMostRecentFourUrls()
{
    kiriview::PredecodeDisplayedHistory history;
    for (int index = 0; index < 6; ++index) {
        history.setDisplayedUrls({ kiriview::TestSupport::indexedImageUrl(index) });
    }

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), kiriview::TestSupport::indexedImageUrl(5));
    QCOMPARE(history.recentUrls().size(), std::size_t(4));
    QCOMPARE(history.recentUrls().at(0), kiriview::TestSupport::indexedImageUrl(4));
    QCOMPARE(history.recentUrls().at(1), kiriview::TestSupport::indexedImageUrl(3));
    QCOMPARE(history.recentUrls().at(2), kiriview::TestSupport::indexedImageUrl(2));
    QCOMPARE(history.recentUrls().at(3), kiriview::TestSupport::indexedImageUrl(1));
    QVERIFY(!history.retainedContains(kiriview::TestSupport::indexedImageUrl(0)));
}

void TestPredecodeDisplayedHistory::clearRemovesCurrentAndRecentHistory()
{
    kiriview::PredecodeDisplayedHistory history;
    history.setDisplayedUrls({ kiriview::TestSupport::indexedImageUrl(1) });
    history.setDisplayedUrls({ kiriview::TestSupport::indexedImageUrl(2) });

    history.clear();

    QVERIFY(history.currentUrls().empty());
    QVERIFY(history.recentUrls().empty());
}

QTEST_GUILESS_MAIN(TestPredecodeDisplayedHistory)

#include "test_predecodedisplayedhistory.moc"
