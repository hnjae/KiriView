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
    void staticTileCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
};

void TestImageCachePolicy::retainsLeastRecentlyUsedEntriesWithinBudget()
{
    using Entry = KiriView::ImageCacheRetentionEntry;

    QCOMPARE(KiriView::lruCacheRetainedIndices(
                 { Entry { 16, 3 }, Entry { 16, 1 }, Entry { 16, 2 } }, 32),
        std::vector<std::size_t>({ 0, 2 }));
    QCOMPARE(KiriView::lruCacheRetainedIndices(
                 { Entry { 60, 3 }, Entry { 50, 2 }, Entry { 40, 1 } }, 100),
        std::vector<std::size_t>({ 0 }));
}

void TestImageCachePolicy::rejectsInvalidEntriesAndBudgets()
{
    using Entry = KiriView::ImageCacheRetentionEntry;

    QCOMPARE(KiriView::lruCacheRetainedIndices(
                 { Entry { 0, 1 }, Entry { 10, 2 }, Entry { -1, 3 } }, 100),
        std::vector<std::size_t>({ 1 }));
    QVERIFY(KiriView::lruCacheRetainedIndices({ Entry { 10, 1 } }, 0).empty());
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
