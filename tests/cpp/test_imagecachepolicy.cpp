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
    void predecodeCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void thumbnailCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void resolvedCacheBudgetsUseSnapshotAndPreserveOverrides();
    void displayImageCacheByteBudgetMirrorsStaticTileBudgetByDefault();
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

void TestImageCachePolicy::predecodeCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = 1024 * 1024 * 1024;

    QCOMPARE(KiriView::predecodeCachePreferredByteBudget(), preferredByteBudget);
    QCOMPARE(KiriView::predecodeCacheByteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(KiriView::predecodeCacheByteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 8);
    QCOMPARE(KiriView::predecodeCacheByteBudgetForSystemMemory(preferredByteBudget * 16),
        preferredByteBudget);
}

void TestImageCachePolicy::thumbnailCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = 64 * 1024 * 1024;

    QCOMPARE(KiriView::thumbnailCachePreferredByteBudget(), preferredByteBudget);
    QCOMPARE(KiriView::thumbnailCacheByteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(KiriView::thumbnailCacheByteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 64);
    QCOMPARE(KiriView::thumbnailCacheByteBudgetForSystemMemory(preferredByteBudget * 128),
        preferredByteBudget);
}

void TestImageCachePolicy::resolvedCacheBudgetsUseSnapshotAndPreserveOverrides()
{
    constexpr qsizetype staticTilePreferredByteBudget = 512 * 1024 * 1024;
    constexpr qsizetype predecodePreferredByteBudget = 1024 * 1024 * 1024;
    constexpr qsizetype thumbnailPreferredByteBudget = 64 * 1024 * 1024;

    const KiriView::ImageCacheBudgets defaultBudgets = KiriView::resolvedImageCacheBudgets(
        KiriView::ImageCacheBudgetRequest {
            0,
            0,
            staticTilePreferredByteBudget,
            0,
        },
        KiriView::SystemMemorySnapshot { predecodePreferredByteBudget });
    QCOMPARE(defaultBudgets.predecodeCacheByteBudget, predecodePreferredByteBudget / 8);
    QCOMPARE(defaultBudgets.staticTileCacheByteBudget, staticTilePreferredByteBudget / 8);
    QCOMPARE(defaultBudgets.thumbnailCacheByteBudget, predecodePreferredByteBudget / 64);
    QCOMPARE(defaultBudgets.displayImageCacheByteBudget, defaultBudgets.staticTileCacheByteBudget);

    const KiriView::ImageCacheBudgets explicitBudgets = KiriView::resolvedImageCacheBudgets(
        KiriView::ImageCacheBudgetRequest {
            4096,
            8192,
            staticTilePreferredByteBudget,
            16384,
            32768,
        },
        KiriView::SystemMemorySnapshot { predecodePreferredByteBudget });
    QCOMPARE(explicitBudgets.predecodeCacheByteBudget, qsizetype(4096));
    QCOMPARE(explicitBudgets.staticTileCacheByteBudget, qsizetype(8192));
    QCOMPARE(explicitBudgets.thumbnailCacheByteBudget, qsizetype(16384));
    QCOMPARE(explicitBudgets.displayImageCacheByteBudget, qsizetype(32768));
}

void TestImageCachePolicy::displayImageCacheByteBudgetMirrorsStaticTileBudgetByDefault()
{
    const KiriView::ImageCacheBudgets budgets = KiriView::resolvedImageCacheBudgets(
        KiriView::ImageCacheBudgetRequest {
            0,
            65536,
            0,
            0,
            0,
        },
        KiriView::SystemMemorySnapshot {});

    QCOMPARE(budgets.staticTileCacheByteBudget, qsizetype(65536));
    QCOMPARE(budgets.displayImageCacheByteBudget, qsizetype(65536));
}

QTEST_GUILESS_MAIN(TestImageCachePolicy)

#include "test_imagecachepolicy.moc"
