// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/predecodepolicyconversion.h"

#include <QObject>
#include <QTest>
#include <cstddef>
#include <vector>

class TestPredecodePolicyConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceProfileMapsPlainFields();
    void momentumModeMapsBothDirections();
    void momentumDirectionMapsBothDirections();
    void policyInputMapsPlainFields();
    void momentumStateMapsPlainFieldsBothDirections();
    void loadStateInputsMapPlainFields();
    void schedulePlanMapsFromRust();
    void queuedLoadPlanMapsFromRust();
};

void TestPredecodePolicyConversion::sourceProfileMapsPlainFields()
{
    const KiriView::RustPredecodeSourceProfile converted
        = KiriView::Bridge::rustPredecodeSourceProfile(
            KiriView::PredecodeSourceProfile { 2, 4, 8, 3 });

    QCOMPARE(converted.neutral_previous_image_count, std::size_t(2));
    QCOMPARE(converted.neutral_next_image_count, std::size_t(4));
    QCOMPARE(converted.biased_direction_image_count, std::size_t(8));
    QCOMPARE(converted.parallel_limit, std::size_t(3));
}

void TestPredecodePolicyConversion::momentumModeMapsBothDirections()
{
    using KiriView::PredecodeMomentumMode;
    using KiriView::RustPredecodeMomentumMode;

    QVERIFY(KiriView::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::Neutral)
        == RustPredecodeMomentumMode::Neutral);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::NextBiased)
        == RustPredecodeMomentumMode::NextBiased);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::PrevBiased)
        == RustPredecodeMomentumMode::PrevBiased);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::ScrubbingNext)
        == RustPredecodeMomentumMode::ScrubbingNext);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::ScrubbingPrev)
        == RustPredecodeMomentumMode::ScrubbingPrev);
    QVERIFY(KiriView::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::Neutral)
        == PredecodeMomentumMode::Neutral);
    QVERIFY(KiriView::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::NextBiased)
        == PredecodeMomentumMode::NextBiased);
    QVERIFY(KiriView::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::PrevBiased)
        == PredecodeMomentumMode::PrevBiased);
    QVERIFY(
        KiriView::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::ScrubbingNext)
        == PredecodeMomentumMode::ScrubbingNext);
    QVERIFY(
        KiriView::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::ScrubbingPrev)
        == PredecodeMomentumMode::ScrubbingPrev);
}

void TestPredecodePolicyConversion::momentumDirectionMapsBothDirections()
{
    using KiriView::PredecodeMomentumDirection;
    using KiriView::RustPredecodeMomentumDirection;

    QVERIFY(KiriView::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::None)
        == RustPredecodeMomentumDirection::None);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::Previous)
        == RustPredecodeMomentumDirection::Previous);
    QVERIFY(KiriView::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::Next)
        == RustPredecodeMomentumDirection::Next);
    QVERIFY(
        KiriView::Bridge::predecodeMomentumDirectionFromRust(RustPredecodeMomentumDirection::None)
        == PredecodeMomentumDirection::None);
    QVERIFY(KiriView::Bridge::predecodeMomentumDirectionFromRust(
                RustPredecodeMomentumDirection::Previous)
        == PredecodeMomentumDirection::Previous);
    QVERIFY(
        KiriView::Bridge::predecodeMomentumDirectionFromRust(RustPredecodeMomentumDirection::Next)
        == PredecodeMomentumDirection::Next);
}

void TestPredecodePolicyConversion::policyInputMapsPlainFields()
{
    const KiriView::RustPredecodePolicyInput converted
        = KiriView::Bridge::rustPredecodePolicyInput(KiriView::PredecodePolicyInput {
            KiriView::PredecodeSourceProfile { 2, 4, 8, 3 },
            KiriView::PredecodeMomentumMode::ScrubbingPrev,
            true,
        });

    QCOMPARE(converted.source_profile.neutral_previous_image_count, std::size_t(2));
    QCOMPARE(converted.source_profile.neutral_next_image_count, std::size_t(4));
    QCOMPARE(converted.source_profile.biased_direction_image_count, std::size_t(8));
    QCOMPARE(converted.source_profile.parallel_limit, std::size_t(3));
    QVERIFY(converted.momentum_mode == KiriView::RustPredecodeMomentumMode::ScrubbingPrev);
    QVERIFY(converted.power_saver_enabled);
}

void TestPredecodePolicyConversion::momentumStateMapsPlainFieldsBothDirections()
{
    const KiriView::RustPredecodeMomentumState rustState
        = KiriView::Bridge::rustPredecodeMomentumState(KiriView::PredecodeMomentumState {
            7,
            1234,
            3,
            KiriView::PredecodeMomentumDirection::Next,
            KiriView::PredecodeMomentumMode::NextBiased,
        });

    QCOMPARE(rustState.last_page_index, 7);
    QCOMPARE(rustState.last_navigation_msec, qint64(1234));
    QCOMPARE(rustState.same_direction_move_count, 3);
    QVERIFY(rustState.last_direction == KiriView::RustPredecodeMomentumDirection::Next);
    QVERIFY(rustState.mode == KiriView::RustPredecodeMomentumMode::NextBiased);

    KiriView::RustPredecodeMomentumState updated {};
    updated.last_page_index = 4;
    updated.last_navigation_msec = 2000;
    updated.same_direction_move_count = 2;
    updated.last_direction = KiriView::RustPredecodeMomentumDirection::Previous;
    updated.mode = KiriView::RustPredecodeMomentumMode::PrevBiased;

    const KiriView::PredecodeMomentumState converted
        = KiriView::Bridge::predecodeMomentumStateFromRust(updated);

    QCOMPARE(converted.lastPageIndex, 4);
    QCOMPARE(converted.lastNavigationMsec, qint64(2000));
    QCOMPARE(converted.sameDirectionMoveCount, 2);
    QVERIFY(converted.lastDirection == KiriView::PredecodeMomentumDirection::Previous);
    QVERIFY(converted.mode == KiriView::PredecodeMomentumMode::PrevBiased);
}

void TestPredecodePolicyConversion::loadStateInputsMapPlainFields()
{
    const KiriView::RustPredecodeCachedImageState cached
        = KiriView::Bridge::rustPredecodeCachedImageState(KiriView::PredecodeCachedImageState {
            true,
            true,
            1,
            2,
            3,
            4096,
        });
    QVERIFY(cached.current_displayed);
    QVERIFY(cached.recent_displayed);
    QCOMPARE(cached.current_displayed_priority, std::size_t(1));
    QCOMPARE(cached.recent_displayed_priority, std::size_t(2));
    QCOMPARE(cached.window_priority, std::size_t(3));
    QCOMPARE(cached.byte_cost, qint64(4096));

    const KiriView::RustPredecodeWindowLoadState window
        = KiriView::Bridge::rustPredecodeWindowLoadState(
            KiriView::PredecodeWindowLoadState { true, false, true });
    QVERIFY(window.displayed);
    QVERIFY(!window.cached);
    QVERIFY(window.in_flight);

    const KiriView::RustPredecodeQueuedLoadState queued
        = KiriView::Bridge::rustPredecodeQueuedLoadState(
            KiriView::PredecodeQueuedLoadState { true, true, false, true });
    QVERIFY(queued.valid);
    QVERIFY(queued.in_window);
    QVERIFY(!queued.cached);
    QVERIFY(queued.in_flight);
}

void TestPredecodePolicyConversion::schedulePlanMapsFromRust()
{
    KiriView::RustPredecodeSchedulePlan rustPlan {};
    rustPlan.parallel_limit = 3;
    rustPlan.target_indices.push_back(5);
    rustPlan.target_indices.push_back(8);

    const KiriView::PredecodeSchedulePlan plan
        = KiriView::Bridge::predecodeSchedulePlanFromRust(rustPlan);

    QCOMPARE(plan.parallelLimit, std::size_t(3));
    QVERIFY(plan.targetIndices == std::vector<std::size_t>({ 5, 8 }));
}

void TestPredecodePolicyConversion::queuedLoadPlanMapsFromRust()
{
    KiriView::RustPredecodeQueuedLoadPlan rustPlan {};
    rustPlan.found = true;
    rustPlan.index = 6;
    rustPlan.discard_count = 2;

    const KiriView::PredecodeQueuedLoadPlan plan
        = KiriView::Bridge::predecodeQueuedLoadPlanFromRust(rustPlan);

    QVERIFY(plan.found);
    QCOMPARE(plan.index, std::size_t(6));
    QCOMPARE(plan.discardCount, std::size_t(2));
}

QTEST_GUILESS_MAIN(TestPredecodePolicyConversion)

#include "test_predecodepolicyconversion.moc"
