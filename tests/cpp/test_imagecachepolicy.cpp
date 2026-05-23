// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "cache/imagecachepolicy.h"

#include <QObject>
#include <QTest>
#include <QtGlobal>
#include <vector>

class TestImageCachePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void retainsLeastRecentlyUsedEntriesWithinBudget();
    void rejectsInvalidEntriesAndBudgets();
    void reportsRetainedByteCosts();
    void staticTileCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
};

void TestImageCachePolicy::retainsLeastRecentlyUsedEntriesWithinBudget()
{
    using Entry = KiriView::ImageCacheRetentionEntry;

    const std::vector<KiriView::ImageCacheRetainedEntry> twoRetained
        = KiriView::lruCacheRetentionPlan(
            { Entry { 16, 3 }, Entry { 16, 1 }, Entry { 16, 2 } }, 32);
    QCOMPARE(twoRetained.size(), std::size_t(2));
    QCOMPARE(twoRetained.at(0).originalIndex, std::size_t(0));
    QCOMPARE(twoRetained.at(1).originalIndex, std::size_t(2));

    const std::vector<KiriView::ImageCacheRetainedEntry> oneRetained
        = KiriView::lruCacheRetentionPlan(
            { Entry { 60, 3 }, Entry { 50, 2 }, Entry { 40, 1 } }, 100);
    QCOMPARE(oneRetained.size(), std::size_t(1));
    QCOMPARE(oneRetained.at(0).originalIndex, std::size_t(0));
}

void TestImageCachePolicy::rejectsInvalidEntriesAndBudgets()
{
    using Entry = KiriView::ImageCacheRetentionEntry;

    const std::vector<KiriView::ImageCacheRetainedEntry> retained = KiriView::lruCacheRetentionPlan(
        { Entry { 0, 1 }, Entry { 10, 2 }, Entry { -1, 3 } }, 100);
    QCOMPARE(retained.size(), std::size_t(1));
    QCOMPARE(retained.at(0).originalIndex, std::size_t(1));
    QVERIFY(KiriView::lruCacheRetentionPlan({ Entry { 10, 1 } }, 0).empty());
}

void TestImageCachePolicy::reportsRetainedByteCosts()
{
    using Entry = KiriView::ImageCacheRetentionEntry;

    const std::vector<KiriView::ImageCacheRetainedEntry> retained = KiriView::lruCacheRetentionPlan(
        { Entry { 18, 3 }, Entry { 16, 1 }, Entry { 14, 2 } }, 32);

    QCOMPARE(retained.size(), std::size_t(2));
    QCOMPARE(retained.at(0).originalIndex, std::size_t(0));
    QCOMPARE(retained.at(0).byteCost, qsizetype(18));
    QCOMPARE(retained.at(1).originalIndex, std::size_t(2));
    QCOMPARE(retained.at(1).byteCost, qsizetype(14));
}

void TestImageCachePolicy::staticTileCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = 512 * 1024 * 1024;

    QCOMPARE(KiriView::staticTileCacheByteBudgetForSystemMemory(0, preferredByteBudget),
        preferredByteBudget);
    QCOMPARE(KiriView::staticTileCacheByteBudgetForSystemMemory(
                 preferredByteBudget, preferredByteBudget),
        preferredByteBudget / 16);
    QCOMPARE(KiriView::staticTileCacheByteBudgetForSystemMemory(
                 preferredByteBudget * 32, preferredByteBudget),
        preferredByteBudget);
    QCOMPARE(
        KiriView::staticTileCacheByteBudgetForSystemMemory(preferredByteBudget, -1), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestImageCachePolicy)

#include "test_imagecachepolicy.moc"
