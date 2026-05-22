// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/navigationindexpolicy.h"

#include <QObject>
#include <QTest>
#include <optional>

class TestNavigationIndexPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adjacentCandidateIndexMovesWithoutWrapping();
    void adjacentCandidateIndexRejectsMissingOrOutOfRangeCurrent();
};

void TestNavigationIndexPolicy::adjacentCandidateIndexMovesWithoutWrapping()
{
    std::optional<std::size_t> target = KiriView::adjacentNavigationCandidateIndex(
        3, std::size_t(1), KiriView::NavigationDirection::Previous);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(0));

    target = KiriView::adjacentNavigationCandidateIndex(
        3, std::size_t(1), KiriView::NavigationDirection::Next);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(2));

    QVERIFY(!KiriView::adjacentNavigationCandidateIndex(
        3, std::size_t(0), KiriView::NavigationDirection::Previous)
            .has_value());
    QVERIFY(!KiriView::adjacentNavigationCandidateIndex(
        3, std::size_t(2), KiriView::NavigationDirection::Next)
            .has_value());
}

void TestNavigationIndexPolicy::adjacentCandidateIndexRejectsMissingOrOutOfRangeCurrent()
{
    QVERIFY(!KiriView::adjacentNavigationCandidateIndex(
        3, std::nullopt, KiriView::NavigationDirection::Next)
            .has_value());
    QVERIFY(!KiriView::adjacentNavigationCandidateIndex(
        3, std::size_t(3), KiriView::NavigationDirection::Previous)
            .has_value());
}

QTEST_GUILESS_MAIN(TestNavigationIndexPolicy)

#include "test_navigationindexpolicy.moc"
