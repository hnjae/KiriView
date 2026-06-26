// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/videodocumentpolicyconversion.h"
#include "kiriview/src/policy/videodocumentpolicy.cxx.h"

#include <QObject>
#include <QTest>
#include <variant>

class TestVideoDocumentPolicyConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void statusSnapshotAndPlanRoundTripThroughRustPolicy();
    void playbackPlanConversionPreservesOrderedBackendEffects();
};

namespace {
template <typename Operation>
const Operation* operationAt(const kiriview::VideoPlaybackControlPlan& plan, std::size_t index)
{
    return std::get_if<Operation>(&plan.backendOperations.at(index));
}
}

void TestVideoDocumentPolicyConversion::statusSnapshotAndPlanRoundTripThroughRustPolicy()
{
    const kiriview::RustVideoDocumentStatusSnapshot snapshot
        = kiriview::Bridge::rustVideoDocumentStatusSnapshot(kiriview::VideoDocumentStatusSnapshot {
            false,
            false,
            true,
            kiriview::VideoMediaStatus::EndOfMedia,
        });

    const kiriview::VideoDocumentStatusPlan plan
        = kiriview::Bridge::videoDocumentStatusPlanFromRust(
            kiriview::rustVideoDocumentStatusPlan(snapshot));

    QCOMPARE(plan.status, kiriview::VideoDocumentStatus::Ready);
    QVERIFY(plan.mediaEnded);
    QVERIFY(plan.clearPlaying);
}

void TestVideoDocumentPolicyConversion::playbackPlanConversionPreservesOrderedBackendEffects()
{
    const kiriview::RustVideoPlaybackControlSnapshot snapshot
        = kiriview::Bridge::rustVideoPlaybackControlSnapshot(
            kiriview::VideoPlaybackControlSnapshot {
                false,
                true,
                false,
                true,
                true,
                10000,
                10000,
            });

    const kiriview::VideoPlaybackControlPlan plan
        = kiriview::Bridge::videoPlaybackControlPlanFromRust(
            kiriview::rustVideoPlaybackPlayPlan(snapshot));

    QCOMPARE(plan.backendOperations.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::EnsureVideoPlaybackBackendOperation>(plan, 0) != nullptr);
    const auto* setPosition = operationAt<kiriview::SetVideoPlaybackPositionOperation>(plan, 1);
    QVERIFY(setPosition != nullptr);
    QCOMPARE(setPosition->position, 0);
    QVERIFY(operationAt<kiriview::PlayVideoPlaybackOperation>(plan, 2) != nullptr);
    QVERIFY(plan.stateDelta.mediaEnded.has_value());
    QCOMPARE(plan.stateDelta.mediaEnded.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 0);
}

QTEST_GUILESS_MAIN(TestVideoDocumentPolicyConversion)

#include "test_videodocumentpolicyconversion.moc"
