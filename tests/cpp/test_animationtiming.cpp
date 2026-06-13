// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/animationtiming.h"

#include <QObject>
#include <QTest>
#include <cstdint>
#include <limits>

class TestAnimationTiming : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void heifFrameDelayRoundsUpDurationToMilliseconds();
    void heifFrameDelayRejectsMissingDurationOrTimescale();
    void heifFrameDelayClampsToTimerRange();
    void apngFrameDelayUsesDefaultDenominatorAndFloorRounding();
    void apngFrameDelayClampsToTimerRange();
    void apngLoopCountMapsZeroToInfiniteAndClampsFiniteLoops();
};

void TestAnimationTiming::heifFrameDelayRoundsUpDurationToMilliseconds()
{
    QCOMPARE(kiriview::heifFrameDelay(1, 24), 42);
    QCOMPARE(kiriview::heifFrameDelay(3, 2), 1500);
}

void TestAnimationTiming::heifFrameDelayRejectsMissingDurationOrTimescale()
{
    QCOMPARE(kiriview::heifFrameDelay(0, 24), 0);
    QCOMPARE(kiriview::heifFrameDelay(1, 0), 0);
}

void TestAnimationTiming::heifFrameDelayClampsToTimerRange()
{
    QCOMPARE(kiriview::heifFrameDelay(std::numeric_limits<std::uint32_t>::max(), 1),
        std::numeric_limits<int>::max());
}

void TestAnimationTiming::apngFrameDelayUsesDefaultDenominatorAndFloorRounding()
{
    QCOMPARE(kiriview::apngFrameDelay(1, 10), 100);
    QCOMPARE(kiriview::apngFrameDelay(1, 0), 10);
    QCOMPARE(kiriview::apngFrameDelay(1, 24), 41);
}

void TestAnimationTiming::apngFrameDelayClampsToTimerRange()
{
    QCOMPARE(kiriview::apngFrameDelay(std::numeric_limits<std::uint32_t>::max(), 1),
        std::numeric_limits<int>::max());
}

void TestAnimationTiming::apngLoopCountMapsZeroToInfiniteAndClampsFiniteLoops()
{
    QCOMPARE(kiriview::apngLoopCountForPlayCount(0), -1);
    QCOMPARE(kiriview::apngLoopCountForPlayCount(1), 0);
    QCOMPARE(kiriview::apngLoopCountForPlayCount(2), 1);
    QCOMPARE(kiriview::apngLoopCountForPlayCount(std::numeric_limits<std::uint32_t>::max()),
        std::numeric_limits<int>::max());
}

QTEST_GUILESS_MAIN(TestAnimationTiming)

#include "test_animationtiming.moc"
