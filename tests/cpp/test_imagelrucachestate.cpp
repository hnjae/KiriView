// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagelrucachestate.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

class TestImageLruCacheState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void evictsLeastRecentlyUsedEntriesToBudget();
    void replacesExistingEntriesWithoutDoubleCounting();
    void rejectsInvalidOrOversizedEntries();
    void clearRemovesEntriesAndByteCost();
};

void TestImageLruCacheState::evictsLeastRecentlyUsedEntriesToBudget()
{
    KiriView::ImageLruCacheState<int, QString> cache(32);

    QVERIFY(cache.insert(1, QStringLiteral("first"), 16));
    QVERIFY(cache.insert(2, QStringLiteral("second"), 16));
    const std::optional<QString> first = cache.find(1);
    QVERIFY(first.has_value());
    QCOMPARE(*first, QStringLiteral("first"));
    QVERIFY(cache.insert(3, QStringLiteral("third"), 16));

    QVERIFY(cache.contains(1));
    QVERIFY(!cache.contains(2));
    QVERIFY(cache.contains(3));
    const std::vector<QString> values = cache.values();
    QCOMPARE(values.size(), std::size_t(2));
    QCOMPARE(values.at(0), QStringLiteral("first"));
    QCOMPARE(values.at(1), QStringLiteral("third"));
    QCOMPARE(cache.byteCost(), qsizetype(32));
}

void TestImageLruCacheState::replacesExistingEntriesWithoutDoubleCounting()
{
    KiriView::ImageLruCacheState<int, QString> cache(32);

    QVERIFY(cache.insert(1, QStringLiteral("first"), 16));
    QVERIFY(cache.insert(1, QStringLiteral("replacement"), 24));

    const std::optional<QString> replacement = cache.find(1);
    QVERIFY(replacement.has_value());
    QCOMPARE(*replacement, QStringLiteral("replacement"));
    QCOMPARE(cache.values().size(), std::size_t(1));
    QCOMPARE(cache.byteCost(), qsizetype(24));
}

void TestImageLruCacheState::rejectsInvalidOrOversizedEntries()
{
    KiriView::ImageLruCacheState<int, QString> cache(16);

    QVERIFY(!cache.insert(1, QStringLiteral("empty"), 0));
    QVERIFY(!cache.insert(2, QStringLiteral("oversized"), 17));

    QVERIFY(!cache.contains(1));
    QVERIFY(!cache.contains(2));
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

void TestImageLruCacheState::clearRemovesEntriesAndByteCost()
{
    KiriView::ImageLruCacheState<int, QString> cache(16);

    QVERIFY(cache.insert(1, QStringLiteral("first"), 16));
    cache.clear();

    QVERIFY(!cache.contains(1));
    QVERIFY(cache.values().empty());
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestImageLruCacheState)

#include "test_imagelrucachestate.moc"
