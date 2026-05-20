// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecachepolicy.h"

#include <QObject>
#include <QTest>
#include <QtGlobal>
#include <vector>

class TestImageCachePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void retainsLeastRecentlyUsedEntriesWithinBudget();
    void rejectsInvalidEntriesBudgetsAndMismatchedInputs();
    void staticTileCacheByteBudgetUsesPreferredLimitAndSystemMemoryCap();
};

void TestImageCachePolicy::retainsLeastRecentlyUsedEntriesWithinBudget()
{
    QCOMPARE(KiriView::lruCacheRetainedIndices({ 16, 16, 16 }, { 3, 1, 2 }, 32),
        std::vector<std::size_t>({ 0, 2 }));
    QCOMPARE(KiriView::lruCacheRetainedIndices({ 60, 50, 40 }, { 3, 2, 1 }, 100),
        std::vector<std::size_t>({ 0 }));
}

void TestImageCachePolicy::rejectsInvalidEntriesBudgetsAndMismatchedInputs()
{
    QCOMPARE(KiriView::lruCacheRetainedIndices({ 0, 10, -1 }, { 1, 2, 3 }, 100),
        std::vector<std::size_t>({ 1 }));
    QVERIFY(KiriView::lruCacheRetainedIndices({ 10 }, { 1 }, 0).empty());
    QVERIFY(KiriView::lruCacheRetainedIndices({ 10, 20 }, { 1 }, 100).empty());
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
