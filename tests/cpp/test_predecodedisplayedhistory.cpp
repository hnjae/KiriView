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
    KiriView::PredecodeDisplayedHistory history;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = KiriView::TestSupport::indexedImageUrl(2);

    history.setDisplayedUrls({ firstUrl, QUrl(), firstUrl, secondUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(2));
    QCOMPARE(history.currentUrls().at(0), KiriView::normalizedImageUrl(firstUrl));
    QCOMPARE(history.currentUrls().at(1), KiriView::normalizedImageUrl(secondUrl));
    QVERIFY(history.recentUrls().empty());
}

void TestPredecodeDisplayedHistory::previousCurrentUrlsMoveToRecentHistory()
{
    KiriView::PredecodeDisplayedHistory history;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = KiriView::TestSupport::indexedImageUrl(2);
    const QUrl thirdUrl = KiriView::TestSupport::indexedImageUrl(3);

    history.setDisplayedUrls({ firstUrl, secondUrl });
    history.setDisplayedUrls({ thirdUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), KiriView::normalizedImageUrl(thirdUrl));
    QCOMPARE(history.recentUrls().size(), std::size_t(2));
    QCOMPARE(history.recentUrls().at(0), KiriView::normalizedImageUrl(secondUrl));
    QCOMPARE(history.recentUrls().at(1), KiriView::normalizedImageUrl(firstUrl));
    QVERIFY(history.currentContains(KiriView::normalizedImageUrl(thirdUrl)));
    QVERIFY(history.recentContains(KiriView::normalizedImageUrl(firstUrl)));
    QCOMPARE(history.currentPriority(KiriView::normalizedImageUrl(thirdUrl)), std::size_t(0));
    QCOMPARE(history.recentPriority(KiriView::normalizedImageUrl(firstUrl)), std::size_t(1));
}

void TestPredecodeDisplayedHistory::redisplayedUrlsAreRemovedFromRecentHistory()
{
    KiriView::PredecodeDisplayedHistory history;
    const QUrl firstUrl = KiriView::TestSupport::indexedImageUrl(1);
    const QUrl secondUrl = KiriView::TestSupport::indexedImageUrl(2);

    history.setDisplayedUrls({ firstUrl });
    history.setDisplayedUrls({ secondUrl });
    history.setDisplayedUrls({ firstUrl });

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), KiriView::normalizedImageUrl(firstUrl));
    QCOMPARE(history.recentUrls().size(), std::size_t(1));
    QCOMPARE(history.recentUrls().front(), KiriView::normalizedImageUrl(secondUrl));
    QVERIFY(history.retainedContains(KiriView::normalizedImageUrl(firstUrl)));
    QVERIFY(history.retainedContains(KiriView::normalizedImageUrl(secondUrl)));
}

void TestPredecodeDisplayedHistory::recentHistoryKeepsOnlyMostRecentFourUrls()
{
    KiriView::PredecodeDisplayedHistory history;
    for (int index = 0; index < 6; ++index) {
        history.setDisplayedUrls({ KiriView::TestSupport::indexedImageUrl(index) });
    }

    QCOMPARE(history.currentUrls().size(), std::size_t(1));
    QCOMPARE(history.currentUrls().front(), KiriView::TestSupport::indexedImageUrl(5));
    QCOMPARE(history.recentUrls().size(), std::size_t(4));
    QCOMPARE(history.recentUrls().at(0), KiriView::TestSupport::indexedImageUrl(4));
    QCOMPARE(history.recentUrls().at(1), KiriView::TestSupport::indexedImageUrl(3));
    QCOMPARE(history.recentUrls().at(2), KiriView::TestSupport::indexedImageUrl(2));
    QCOMPARE(history.recentUrls().at(3), KiriView::TestSupport::indexedImageUrl(1));
    QVERIFY(!history.retainedContains(KiriView::TestSupport::indexedImageUrl(0)));
}

void TestPredecodeDisplayedHistory::clearRemovesCurrentAndRecentHistory()
{
    KiriView::PredecodeDisplayedHistory history;
    history.setDisplayedUrls({ KiriView::TestSupport::indexedImageUrl(1) });
    history.setDisplayedUrls({ KiriView::TestSupport::indexedImageUrl(2) });

    history.clear();

    QVERIFY(history.currentUrls().empty());
    QVERIFY(history.recentUrls().empty());
}

QTEST_GUILESS_MAIN(TestPredecodeDisplayedHistory)

#include "test_predecodedisplayedhistory.moc"
