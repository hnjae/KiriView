// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/predecodecache.h"
#include "predecode/predecodecachebudget.h"

#include <QObject>
#include <QTest>

class TestPredecodeCacheBudget : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultBudgetUsesSystemMemorySnapshot();
    void explicitPositiveBudgetIsPreserved();
    void nonPositiveBudgetResolvesToDefault();
};

void TestPredecodeCacheBudget::defaultBudgetUsesSystemMemorySnapshot()
{
    constexpr qsizetype preferredByteBudget = 1024 * 1024 * 1024;

    QCOMPARE(KiriView::defaultPredecodeCacheByteBudget(KiriView::SystemMemorySnapshot {}),
        KiriView::PredecodeCache::byteBudgetForSystemMemory(0));
    QCOMPARE(KiriView::defaultPredecodeCacheByteBudget(
                 KiriView::SystemMemorySnapshot { preferredByteBudget }),
        KiriView::PredecodeCache::byteBudgetForSystemMemory(preferredByteBudget));
}

void TestPredecodeCacheBudget::explicitPositiveBudgetIsPreserved()
{
    QCOMPARE(KiriView::resolvedPredecodeCacheByteBudget(
                 4096, KiriView::SystemMemorySnapshot { 1024 * 1024 * 1024 }),
        qsizetype(4096));
}

void TestPredecodeCacheBudget::nonPositiveBudgetResolvesToDefault()
{
    constexpr qsizetype preferredByteBudget = 1024 * 1024 * 1024;
    const KiriView::SystemMemorySnapshot systemMemory { preferredByteBudget };
    const qsizetype defaultBudget = KiriView::defaultPredecodeCacheByteBudget(systemMemory);

    QCOMPARE(KiriView::resolvedPredecodeCacheByteBudget(0, systemMemory), defaultBudget);
    QCOMPARE(KiriView::resolvedPredecodeCacheByteBudget(-1, systemMemory), defaultBudget);
}

QTEST_GUILESS_MAIN(TestPredecodeCacheBudget)

#include "test_predecodecachebudget.moc"
