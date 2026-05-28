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
using KiriView::PredecodeMomentumDirection;
using KiriView::PredecodeMomentumMode;
using KiriView::PredecodeMomentumState;
using KiriView::PredecodePolicyInput;
using KiriView::PredecodeSchedulePlan;
using KiriView::PredecodeSourceProfile;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::localUrl;

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
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/book.cbz")));
    QVERIFY(openedCollectionScope.has_value());
    const PredecodeSourceProfile archiveProfile
        = KiriView::predecodeSourceProfileForOpenedCollectionScope(*openedCollectionScope, 8);

    const PredecodeSchedulePlan plan
        = KiriView::predecodeSchedulePlan(15, 5, policyInput(archiveProfile));

    QCOMPARE(plan.parallelLimit, std::size_t(4));
    QVERIFY(plan.targetIndices == std::vector<std::size_t>({ 5, 6, 4, 7, 3, 8, 9 }));

    const PredecodeSchedulePlan missingCurrent = KiriView::predecodeSchedulePlan(
        15, std::nullopt, policyInput(KiriView::directMediaPredecodeSourceProfile()));
    QCOMPARE(missingCurrent.parallelLimit, std::size_t(1));
    QVERIFY(missingCurrent.targetIndices.empty());
}

void TestPredecodePolicy::openedCollectionSourceProfilesPreserveRuntimeTuning()
{
    const KiriView::OpenedCollectionScopeLocation directoryCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), KiriView::OpenedCollectionScopeKind::Directory);
    const PredecodeSourceProfile directoryProfile
        = KiriView::predecodeSourceProfileForOpenedCollectionScope(directoryCollection, 8);
    QCOMPARE(directoryProfile.neutralPreviousImageCount, std::size_t(2));
    QCOMPARE(directoryProfile.neutralNextImageCount, std::size_t(4));
    QCOMPARE(directoryProfile.biasedDirectionImageCount, std::size_t(6));
    QCOMPARE(directoryProfile.parallelLimit, std::size_t(2));

    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/book.cbz")));
    QVERIFY(openedCollectionScope.has_value());
    const PredecodeSourceProfile archiveProfile
        = KiriView::predecodeSourceProfileForOpenedCollectionScope(*openedCollectionScope, 8);
    QCOMPARE(archiveProfile.neutralPreviousImageCount, std::size_t(2));
    QCOMPARE(archiveProfile.neutralNextImageCount, std::size_t(4));
    QCOMPARE(archiveProfile.biasedDirectionImageCount, std::size_t(8));
    QCOMPARE(archiveProfile.parallelLimit, std::size_t(4));
}

void TestPredecodePolicy::momentumStateRoundTripsThroughPolicyBoundary()
{
    PredecodeMomentumState state
        = KiriView::predecodeUpdatedMomentumState(PredecodeMomentumState {}, 4, 1000);
    QCOMPARE(state.lastPageIndex, 4);
    QCOMPARE(state.lastNavigationMsec, qint64(1000));
    QCOMPARE(state.sameDirectionMoveCount, 0);
    QVERIFY(state.lastDirection == PredecodeMomentumDirection::None);
    QVERIFY(state.mode == PredecodeMomentumMode::Neutral);

    state = KiriView::predecodeUpdatedMomentumState(state, 5, 1300);
    state = KiriView::predecodeUpdatedMomentumState(state, 6, 1600);

    QCOMPARE(state.lastPageIndex, 6);
    QCOMPARE(state.sameDirectionMoveCount, 2);
    QVERIFY(state.lastDirection == PredecodeMomentumDirection::Next);
    QVERIFY(state.mode == PredecodeMomentumMode::NextBiased);
}

QTEST_GUILESS_MAIN(TestPredecodePolicy)

#include "test_predecodepolicy.moc"
