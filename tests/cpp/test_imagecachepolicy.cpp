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
    void displayImageCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void displayImageCachePreferredByteBudgetIsNamedPolicyDefault();
    void predecodeCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void thumbnailCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
    void resolvedCacheBudgetsUseSnapshotAndPreserveOverrides();
    void resolvedCacheBudgetsUseDisplayImagePreferredDefault();
    void resolvedCacheBudgetsUseExplicitDisplayImageOverride();
};

void TestImageCachePolicy::retainsLeastRecentlyUsedEntriesWithinBudget()
{
    using Entry = kiriview::ImageCacheRetentionEntry;

    const std::vector<kiriview::ImageCacheRetainedEntry> twoRetained
        = kiriview::lruCacheRetentionPlan(
            { Entry { 16, 3 }, Entry { 16, 1 }, Entry { 16, 2 } }, 32);
    QCOMPARE(twoRetained.size(), std::size_t(2));
    QCOMPARE(twoRetained.at(0).originalIndex, std::size_t(0));
    QCOMPARE(twoRetained.at(1).originalIndex, std::size_t(2));

    const std::vector<kiriview::ImageCacheRetainedEntry> oneRetained
        = kiriview::lruCacheRetentionPlan(
            { Entry { 60, 3 }, Entry { 50, 2 }, Entry { 40, 1 } }, 100);
    QCOMPARE(oneRetained.size(), std::size_t(1));
    QCOMPARE(oneRetained.at(0).originalIndex, std::size_t(0));
}

void TestImageCachePolicy::rejectsInvalidEntriesAndBudgets()
{
    using Entry = kiriview::ImageCacheRetentionEntry;

    const std::vector<kiriview::ImageCacheRetainedEntry> retained = kiriview::lruCacheRetentionPlan(
        { Entry { 0, 1 }, Entry { 10, 2 }, Entry { -1, 3 } }, 100);
    QCOMPARE(retained.size(), std::size_t(1));
    QCOMPARE(retained.at(0).originalIndex, std::size_t(1));
    QVERIFY(kiriview::lruCacheRetentionPlan({ Entry { 10, 1 } }, 0).empty());
}

void TestImageCachePolicy::reportsRetainedByteCosts()
{
    using Entry = kiriview::ImageCacheRetentionEntry;

    const std::vector<kiriview::ImageCacheRetainedEntry> retained = kiriview::lruCacheRetentionPlan(
        { Entry { 18, 3 }, Entry { 16, 1 }, Entry { 14, 2 } }, 32);

    QCOMPARE(retained.size(), std::size_t(2));
    QCOMPARE(retained.at(0).originalIndex, std::size_t(0));
    QCOMPARE(retained.at(0).byteCost, qsizetype(18));
    QCOMPARE(retained.at(1).originalIndex, std::size_t(2));
    QCOMPARE(retained.at(1).byteCost, qsizetype(14));
}

void TestImageCachePolicy::displayImageCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    const qsizetype preferredByteBudget = kiriview::displayImageCachePreferredByteBudget();

    QCOMPARE(kiriview::displayImageCacheByteBudgetForSystemMemory(0, preferredByteBudget),
        preferredByteBudget);
    QCOMPARE(kiriview::displayImageCacheByteBudgetForSystemMemory(
                 preferredByteBudget, preferredByteBudget),
        preferredByteBudget / 16);
    QCOMPARE(kiriview::displayImageCacheByteBudgetForSystemMemory(
                 preferredByteBudget * 32, preferredByteBudget),
        preferredByteBudget);
    QCOMPARE(kiriview::displayImageCacheByteBudgetForSystemMemory(preferredByteBudget, -1),
        qsizetype(0));
}

void TestImageCachePolicy::displayImageCachePreferredByteBudgetIsNamedPolicyDefault()
{
    constexpr qsizetype preferredByteBudget = 512 * 1024 * 1024;

    QCOMPARE(kiriview::displayImageCachePreferredByteBudget(), preferredByteBudget);
}

void TestImageCachePolicy::predecodeCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = 1024 * 1024 * 1024;

    QCOMPARE(kiriview::predecodeCachePreferredByteBudget(), preferredByteBudget);
    QCOMPARE(kiriview::predecodeCacheByteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(kiriview::predecodeCacheByteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 8);
    QCOMPARE(kiriview::predecodeCacheByteBudgetForSystemMemory(preferredByteBudget * 16),
        preferredByteBudget);
}

void TestImageCachePolicy::thumbnailCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap()
{
    constexpr qsizetype preferredByteBudget = 64 * 1024 * 1024;

    QCOMPARE(kiriview::thumbnailCachePreferredByteBudget(), preferredByteBudget);
    QCOMPARE(kiriview::thumbnailCacheByteBudgetForSystemMemory(0), preferredByteBudget);
    QCOMPARE(kiriview::thumbnailCacheByteBudgetForSystemMemory(preferredByteBudget),
        preferredByteBudget / 64);
    QCOMPARE(kiriview::thumbnailCacheByteBudgetForSystemMemory(preferredByteBudget * 128),
        preferredByteBudget);
}

void TestImageCachePolicy::resolvedCacheBudgetsUseSnapshotAndPreserveOverrides()
{
    constexpr qsizetype displayImagePreferredByteBudget = 512 * 1024 * 1024;
    constexpr qsizetype predecodePreferredByteBudget = 1024 * 1024 * 1024;
    constexpr qsizetype thumbnailPreferredByteBudget = 64 * 1024 * 1024;

    const kiriview::ImageCacheBudgets defaultBudgets = kiriview::resolvedImageCacheBudgets(
        kiriview::ImageCacheBudgetRequest {
            0,
            0,
            displayImagePreferredByteBudget,
            0,
        },
        kiriview::SystemMemorySnapshot { predecodePreferredByteBudget });
    QCOMPARE(defaultBudgets.predecodeCacheByteBudget, predecodePreferredByteBudget / 8);
    QCOMPARE(defaultBudgets.displayImageCacheByteBudget, displayImagePreferredByteBudget / 8);
    QCOMPARE(defaultBudgets.thumbnailCacheByteBudget, predecodePreferredByteBudget / 64);

    const kiriview::ImageCacheBudgets explicitBudgets = kiriview::resolvedImageCacheBudgets(
        kiriview::ImageCacheBudgetRequest {
            4096,
            32768,
            displayImagePreferredByteBudget,
            16384,
        },
        kiriview::SystemMemorySnapshot { predecodePreferredByteBudget });
    QCOMPARE(explicitBudgets.predecodeCacheByteBudget, qsizetype(4096));
    QCOMPARE(explicitBudgets.displayImageCacheByteBudget, qsizetype(32768));
    QCOMPARE(explicitBudgets.thumbnailCacheByteBudget, qsizetype(16384));
}

void TestImageCachePolicy::resolvedCacheBudgetsUseDisplayImagePreferredDefault()
{
    const qsizetype preferredByteBudget = kiriview::displayImageCachePreferredByteBudget();

    const kiriview::ImageCacheBudgets budgets
        = kiriview::resolvedImageCacheBudgets(kiriview::ImageCacheBudgetRequest {},
            kiriview::SystemMemorySnapshot {
                preferredByteBudget * 32,
            });

    QCOMPARE(budgets.displayImageCacheByteBudget, preferredByteBudget);
}

void TestImageCachePolicy::resolvedCacheBudgetsUseExplicitDisplayImageOverride()
{
    const kiriview::ImageCacheBudgets budgets = kiriview::resolvedImageCacheBudgets(
        kiriview::ImageCacheBudgetRequest {
            0,
            65536,
            0,
            0,
        },
        kiriview::SystemMemorySnapshot {});

    QCOMPARE(budgets.displayImageCacheByteBudget, qsizetype(65536));
}

QTEST_GUILESS_MAIN(TestImageCachePolicy)

#include "test_imagecachepolicy.moc"
