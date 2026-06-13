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
    const kiriview::RustPredecodeSourceProfile converted
        = kiriview::Bridge::rustPredecodeSourceProfile(
            kiriview::PredecodeSourceProfile { 2, 4, 8, 3 });

    QCOMPARE(converted.neutral_previous_image_count, std::size_t(2));
    QCOMPARE(converted.neutral_next_image_count, std::size_t(4));
    QCOMPARE(converted.biased_direction_image_count, std::size_t(8));
    QCOMPARE(converted.parallel_limit, std::size_t(3));
}

void TestPredecodePolicyConversion::momentumModeMapsBothDirections()
{
    using kiriview::PredecodeMomentumMode;
    using kiriview::RustPredecodeMomentumMode;

    QVERIFY(kiriview::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::Neutral)
        == RustPredecodeMomentumMode::Neutral);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::NextBiased)
        == RustPredecodeMomentumMode::NextBiased);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::PrevBiased)
        == RustPredecodeMomentumMode::PrevBiased);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::ScrubbingNext)
        == RustPredecodeMomentumMode::ScrubbingNext);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumMode(PredecodeMomentumMode::ScrubbingPrev)
        == RustPredecodeMomentumMode::ScrubbingPrev);
    QVERIFY(kiriview::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::Neutral)
        == PredecodeMomentumMode::Neutral);
    QVERIFY(kiriview::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::NextBiased)
        == PredecodeMomentumMode::NextBiased);
    QVERIFY(kiriview::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::PrevBiased)
        == PredecodeMomentumMode::PrevBiased);
    QVERIFY(
        kiriview::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::ScrubbingNext)
        == PredecodeMomentumMode::ScrubbingNext);
    QVERIFY(
        kiriview::Bridge::predecodeMomentumModeFromRust(RustPredecodeMomentumMode::ScrubbingPrev)
        == PredecodeMomentumMode::ScrubbingPrev);
}

void TestPredecodePolicyConversion::momentumDirectionMapsBothDirections()
{
    using kiriview::PredecodeMomentumDirection;
    using kiriview::RustPredecodeMomentumDirection;

    QVERIFY(kiriview::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::None)
        == RustPredecodeMomentumDirection::None);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::Previous)
        == RustPredecodeMomentumDirection::Previous);
    QVERIFY(kiriview::Bridge::rustPredecodeMomentumDirection(PredecodeMomentumDirection::Next)
        == RustPredecodeMomentumDirection::Next);
    QVERIFY(
        kiriview::Bridge::predecodeMomentumDirectionFromRust(RustPredecodeMomentumDirection::None)
        == PredecodeMomentumDirection::None);
    QVERIFY(kiriview::Bridge::predecodeMomentumDirectionFromRust(
                RustPredecodeMomentumDirection::Previous)
        == PredecodeMomentumDirection::Previous);
    QVERIFY(
        kiriview::Bridge::predecodeMomentumDirectionFromRust(RustPredecodeMomentumDirection::Next)
        == PredecodeMomentumDirection::Next);
}

void TestPredecodePolicyConversion::policyInputMapsPlainFields()
{
    const kiriview::RustPredecodePolicyInput converted
        = kiriview::Bridge::rustPredecodePolicyInput(kiriview::PredecodePolicyInput {
            kiriview::PredecodeSourceProfile { 2, 4, 8, 3 },
            kiriview::PredecodeMomentumMode::ScrubbingPrev,
            true,
        });

    QCOMPARE(converted.source_profile.neutral_previous_image_count, std::size_t(2));
    QCOMPARE(converted.source_profile.neutral_next_image_count, std::size_t(4));
    QCOMPARE(converted.source_profile.biased_direction_image_count, std::size_t(8));
    QCOMPARE(converted.source_profile.parallel_limit, std::size_t(3));
    QVERIFY(converted.momentum_mode == kiriview::RustPredecodeMomentumMode::ScrubbingPrev);
    QVERIFY(converted.power_saver_enabled);
}

void TestPredecodePolicyConversion::momentumStateMapsPlainFieldsBothDirections()
{
    const kiriview::RustPredecodeMomentumState rustState
        = kiriview::Bridge::rustPredecodeMomentumState(kiriview::PredecodeMomentumState {
            7,
            1234,
            3,
            kiriview::PredecodeMomentumDirection::Next,
            kiriview::PredecodeMomentumMode::NextBiased,
        });

    QCOMPARE(rustState.last_page_index, 7);
    QCOMPARE(rustState.last_navigation_msec, qint64(1234));
    QCOMPARE(rustState.same_direction_move_count, 3);
    QVERIFY(rustState.last_direction == kiriview::RustPredecodeMomentumDirection::Next);
    QVERIFY(rustState.mode == kiriview::RustPredecodeMomentumMode::NextBiased);

    kiriview::RustPredecodeMomentumState updated {};
    updated.last_page_index = 4;
    updated.last_navigation_msec = 2000;
    updated.same_direction_move_count = 2;
    updated.last_direction = kiriview::RustPredecodeMomentumDirection::Previous;
    updated.mode = kiriview::RustPredecodeMomentumMode::PrevBiased;

    const kiriview::PredecodeMomentumState converted
        = kiriview::Bridge::predecodeMomentumStateFromRust(updated);

    QCOMPARE(converted.lastPageIndex, 4);
    QCOMPARE(converted.lastNavigationMsec, qint64(2000));
    QCOMPARE(converted.sameDirectionMoveCount, 2);
    QVERIFY(converted.lastDirection == kiriview::PredecodeMomentumDirection::Previous);
    QVERIFY(converted.mode == kiriview::PredecodeMomentumMode::PrevBiased);
}

void TestPredecodePolicyConversion::loadStateInputsMapPlainFields()
{
    const kiriview::RustPredecodeCachedImageState cached
        = kiriview::Bridge::rustPredecodeCachedImageState(kiriview::PredecodeCachedImageState {
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

    const kiriview::RustPredecodeWindowLoadState window
        = kiriview::Bridge::rustPredecodeWindowLoadState(
            kiriview::PredecodeWindowLoadState { true, false, true });
    QVERIFY(window.displayed);
    QVERIFY(!window.cached);
    QVERIFY(window.in_flight);

    const kiriview::RustPredecodeQueuedLoadState queued
        = kiriview::Bridge::rustPredecodeQueuedLoadState(
            kiriview::PredecodeQueuedLoadState { true, true, false, true });
    QVERIFY(queued.valid);
    QVERIFY(queued.in_window);
    QVERIFY(!queued.cached);
    QVERIFY(queued.in_flight);
}

void TestPredecodePolicyConversion::schedulePlanMapsFromRust()
{
    kiriview::RustPredecodeSchedulePlan rustPlan {};
    rustPlan.parallel_limit = 3;
    rustPlan.target_indices.push_back(5);
    rustPlan.target_indices.push_back(8);

    const kiriview::PredecodeSchedulePlan plan
        = kiriview::Bridge::predecodeSchedulePlanFromRust(rustPlan);

    QCOMPARE(plan.parallelLimit, std::size_t(3));
    QVERIFY(plan.targetIndices == std::vector<std::size_t>({ 5, 8 }));
}

void TestPredecodePolicyConversion::queuedLoadPlanMapsFromRust()
{
    kiriview::RustPredecodeQueuedLoadPlan rustPlan {};
    rustPlan.found = true;
    rustPlan.index = 6;
    rustPlan.discard_count = 2;

    const kiriview::PredecodeQueuedLoadPlan plan
        = kiriview::Bridge::predecodeQueuedLoadPlanFromRust(rustPlan);

    QVERIFY(plan.found);
    QCOMPARE(plan.index, std::size_t(6));
    QCOMPARE(plan.discardCount, std::size_t(2));
}

QTEST_GUILESS_MAIN(TestPredecodePolicyConversion)

#include "test_predecodepolicyconversion.moc"
