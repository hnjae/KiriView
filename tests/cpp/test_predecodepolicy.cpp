// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecode/predecodepolicy.h"

#include "candidate_test_support.h"
#include "imagecontainer.h"

#include <QObject>
#include <QTest>
#include <optional>
#include <vector>

namespace {
using KiriView::PredecodeDocumentKind;
using KiriView::PredecodeMomentumDirection;
using KiriView::PredecodeMomentumMode;
using KiriView::PredecodeMomentumState;
using KiriView::PredecodePolicyInput;
using KiriView::PredecodeSchedulePlan;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::localUrl;

PredecodePolicyInput policyInput(
    PredecodeDocumentKind kind, PredecodeMomentumMode mode = PredecodeMomentumMode::Neutral)
{
    return PredecodePolicyInput { kind, mode, false, 8 };
}
}

class TestPredecodePolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schedulePlanUsesCppCandidateSnapshot();
    void archiveDocumentInputClassifiesRuntimeLocation();
    void momentumStateRoundTripsThroughPolicyBoundary();
};

void TestPredecodePolicy::schedulePlanUsesCppCandidateSnapshot()
{
    const PredecodeSchedulePlan plan = KiriView::predecodeSchedulePlan(
        15, 5, policyInput(PredecodeDocumentKind::ArchiveDocument));

    QCOMPARE(plan.parallelLimit, std::size_t(4));
    QVERIFY(plan.targetIndices == std::vector<std::size_t>({ 5, 6, 4, 7, 3, 8, 9 }));

    const PredecodeSchedulePlan missingCurrent = KiriView::predecodeSchedulePlan(
        15, std::nullopt, policyInput(PredecodeDocumentKind::Regular));
    QCOMPARE(missingCurrent.parallelLimit, std::size_t(1));
    QVERIFY(missingCurrent.targetIndices.empty());
}

void TestPredecodePolicy::archiveDocumentInputClassifiesRuntimeLocation()
{
    const KiriView::ArchiveDocumentLocation directoryDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            imagesDirectoryUrl(), imagesDirectoryUrl(), KiriView::ArchiveDocumentKind::Directory);
    const PredecodePolicyInput directoryInput = KiriView::predecodePolicyInputForArchiveDocument(
        directoryDocument, PredecodeMomentumMode::Neutral, false, 8);
    QVERIFY(directoryInput.documentKind == PredecodeDocumentKind::DirectoryDocument);

    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(
            localUrl(QStringLiteral("/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const PredecodePolicyInput archiveInput = KiriView::predecodePolicyInputForArchiveDocument(
        *archiveDocument, PredecodeMomentumMode::Neutral, false, 8);
    QVERIFY(archiveInput.documentKind == PredecodeDocumentKind::ArchiveDocument);
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
