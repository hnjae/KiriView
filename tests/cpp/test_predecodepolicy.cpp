// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/predecodepolicy.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <vector>

namespace {
using kiriview::PredecodeMomentumDirection;
using kiriview::PredecodeMomentumMode;
using kiriview::PredecodeMomentumState;
using kiriview::PredecodePolicyInput;
using kiriview::PredecodeSchedulePlan;
using kiriview::PredecodeSourceProfile;
using kiriview::TestSupport::imagesDirectoryUrl;
using kiriview::TestSupport::localUrl;

PredecodePolicyInput policyInput(
    PredecodeSourceProfile profile, PredecodeMomentumMode mode = PredecodeMomentumMode::Neutral)
{
    return PredecodePolicyInput { profile, mode, false };
}
}

class TestPredecodePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schedulePlanUsesCppCandidateSnapshot();
    void openedCollectionSourceProfilesPreserveRuntimeTuning();
    void momentumStateRoundTripsThroughPolicyBoundary();
};

void TestPredecodePolicy::schedulePlanUsesCppCandidateSnapshot()
{
    const PredecodeSourceProfile directProfile = kiriview::directMediaPredecodeSourceProfile();
    const PredecodeSchedulePlan directNeutralPlan
        = kiriview::predecodeSchedulePlan(15, 5, policyInput(directProfile));
    QCOMPARE(directNeutralPlan.parallelLimit, std::size_t(1));
    QVERIFY(directNeutralPlan.targetIndices == std::vector<std::size_t>({ 5, 6, 4, 7, 3 }));

    const PredecodeSchedulePlan directBiasedPlan = kiriview::predecodeSchedulePlan(
        15, 5, policyInput(directProfile, PredecodeMomentumMode::NextBiased));
    QCOMPARE(directBiasedPlan.parallelLimit, std::size_t(1));
    QVERIFY(directBiasedPlan.targetIndices == std::vector<std::size_t>({ 5, 6, 4, 7, 8 }));

    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/book.cbz")));
    QVERIFY(openedCollectionScope.has_value());
    const PredecodeSourceProfile archiveProfile
        = kiriview::predecodeSourceProfileForOpenedCollectionScope(*openedCollectionScope, 8);

    const PredecodeSchedulePlan plan
        = kiriview::predecodeSchedulePlan(15, 5, policyInput(archiveProfile));

    QCOMPARE(plan.parallelLimit, std::size_t(4));
    QVERIFY(plan.targetIndices == std::vector<std::size_t>({ 5, 6, 4, 7, 3, 8, 2, 9 }));

    const PredecodeSchedulePlan missingCurrent = kiriview::predecodeSchedulePlan(
        15, std::nullopt, policyInput(kiriview::directMediaPredecodeSourceProfile()));
    QCOMPARE(missingCurrent.parallelLimit, std::size_t(1));
    QVERIFY(missingCurrent.targetIndices.empty());
}

void TestPredecodePolicy::openedCollectionSourceProfilesPreserveRuntimeTuning()
{
    const kiriview::OpenedCollectionScopeLocation directoryCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), kiriview::OpenedCollectionScopeKind::Directory);
    const PredecodeSourceProfile directoryProfile
        = kiriview::predecodeSourceProfileForOpenedCollectionScope(directoryCollection, 8);
    QCOMPARE(directoryProfile.neutralPreviousPageCount, std::size_t(3));
    QCOMPARE(directoryProfile.neutralNextPageCount, std::size_t(4));
    QCOMPARE(directoryProfile.biasedDirectionPageCount, std::size_t(6));
    QCOMPARE(directoryProfile.parallelLimit, std::size_t(2));

    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/book.cbz")));
    QVERIFY(openedCollectionScope.has_value());
    const PredecodeSourceProfile archiveProfile
        = kiriview::predecodeSourceProfileForOpenedCollectionScope(*openedCollectionScope, 8);
    QCOMPARE(archiveProfile.neutralPreviousPageCount, std::size_t(3));
    QCOMPARE(archiveProfile.neutralNextPageCount, std::size_t(4));
    QCOMPARE(archiveProfile.biasedDirectionPageCount, std::size_t(6));
    QCOMPARE(archiveProfile.parallelLimit, std::size_t(4));
}

void TestPredecodePolicy::momentumStateRoundTripsThroughPolicyBoundary()
{
    PredecodeMomentumState state
        = kiriview::predecodeUpdatedMomentumState(PredecodeMomentumState {}, 4, 1000);
    QCOMPARE(state.lastPageIndex, 4);
    QCOMPARE(state.lastNavigationMsec, qint64(1000));
    QCOMPARE(state.sameDirectionMoveCount, 0);
    QVERIFY(state.lastDirection == PredecodeMomentumDirection::None);
    QVERIFY(state.mode == PredecodeMomentumMode::Neutral);

    state = kiriview::predecodeUpdatedMomentumState(state, 5, 1300);
    state = kiriview::predecodeUpdatedMomentumState(state, 6, 1600);

    QCOMPARE(state.lastPageIndex, 6);
    QCOMPARE(state.sameDirectionMoveCount, 2);
    QVERIFY(state.lastDirection == PredecodeMomentumDirection::Next);
    QVERIFY(state.mode == PredecodeMomentumMode::NextBiased);
}

QTEST_GUILESS_MAIN(TestPredecodePolicy)

#include "test_predecodepolicy.moc"
