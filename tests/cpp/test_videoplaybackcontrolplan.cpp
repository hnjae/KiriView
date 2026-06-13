// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackcontrolplan.h"

#include <QObject>
#include <QTest>
#include <variant>

class TestVideoPlaybackControlPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void playNoopsWithoutSource();
    void playRestartsSeekableEndOfMediaBeforePlaying();
    void pauseRequiresExistingBackend();
    void toggleDispatchesFromCurrentPlaybackState();
    void stopDoesNotCreateBackend();
    void setPositionClampsToKnownDuration();
    void seekByClampsAndNoopsWhenUnchangedOrUnseekable();
};

namespace {
kiriview::VideoPlaybackControlSnapshot playableSnapshot()
{
    return kiriview::VideoPlaybackControlSnapshot {
        false,
        true,
        false,
        false,
        true,
        5000,
        10000,
    };
}

template <typename Operation>
const Operation *operationAt(const kiriview::VideoPlaybackControlPlan &plan, std::size_t index)
{
    return std::get_if<Operation>(&plan.backendOperations.at(index));
}
}

void TestVideoPlaybackControlPlan::playNoopsWithoutSource()
{
    const kiriview::VideoPlaybackControlPlan plan
        = kiriview::videoPlaybackPlayPlan(kiriview::VideoPlaybackControlSnapshot {});

    QVERIFY(plan.isEmpty());
}

void TestVideoPlaybackControlPlan::playRestartsSeekableEndOfMediaBeforePlaying()
{
    kiriview::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.mediaEnded = true;
    snapshot.position = 10000;

    const kiriview::VideoPlaybackControlPlan plan = kiriview::videoPlaybackPlayPlan(snapshot);

    QCOMPARE(plan.backendOperations.size(), std::size_t(3));
    QVERIFY(operationAt<kiriview::EnsureVideoPlaybackBackendOperation>(plan, 0) != nullptr);
    const auto *setPosition = operationAt<kiriview::SetVideoPlaybackPositionOperation>(plan, 1);
    QVERIFY(setPosition != nullptr);
    QCOMPARE(setPosition->position, 0);
    QVERIFY(operationAt<kiriview::PlayVideoPlaybackOperation>(plan, 2) != nullptr);
    QVERIFY(plan.stateDelta.mediaEnded.has_value());
    QCOMPARE(plan.stateDelta.mediaEnded.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 0);
    QVERIFY(!plan.stateDelta.playing.has_value());
}

void TestVideoPlaybackControlPlan::pauseRequiresExistingBackend()
{
    kiriview::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.mediaBackendAvailable = false;
    QVERIFY(kiriview::videoPlaybackPausePlan(snapshot).isEmpty());

    snapshot.mediaBackendAvailable = true;
    const kiriview::VideoPlaybackControlPlan plan = kiriview::videoPlaybackPausePlan(snapshot);

    QCOMPARE(plan.backendOperations.size(), std::size_t(1));
    QVERIFY(operationAt<kiriview::PauseVideoPlaybackOperation>(plan, 0) != nullptr);
    QVERIFY(!plan.stateDelta.playing.has_value());
}

void TestVideoPlaybackControlPlan::toggleDispatchesFromCurrentPlaybackState()
{
    kiriview::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.playing = true;

    kiriview::VideoPlaybackControlPlan plan = kiriview::videoPlaybackTogglePlan(snapshot);
    QCOMPARE(plan.backendOperations.size(), std::size_t(1));
    QVERIFY(operationAt<kiriview::PauseVideoPlaybackOperation>(plan, 0) != nullptr);

    snapshot.playing = false;
    plan = kiriview::videoPlaybackTogglePlan(snapshot);
    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::EnsureVideoPlaybackBackendOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::PlayVideoPlaybackOperation>(plan, 1) != nullptr);
}

void TestVideoPlaybackControlPlan::stopDoesNotCreateBackend()
{
    kiriview::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    kiriview::VideoPlaybackControlPlan plan = kiriview::videoPlaybackStopPlan(snapshot);

    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::StopVideoPlaybackOperation>(plan, 0) != nullptr);
    const auto *setPosition = operationAt<kiriview::SetVideoPlaybackPositionOperation>(plan, 1);
    QVERIFY(setPosition != nullptr);
    QCOMPARE(setPosition->position, 0);

    snapshot.mediaBackendAvailable = false;
    snapshot.playing = true;
    snapshot.position = 4000;

    plan = kiriview::videoPlaybackStopPlan(snapshot);

    QVERIFY(plan.backendOperations.empty());
    QVERIFY(plan.stateDelta.mediaEnded.has_value());
    QCOMPARE(plan.stateDelta.mediaEnded.value(), false);
    QVERIFY(plan.stateDelta.playing.has_value());
    QCOMPARE(plan.stateDelta.playing.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 0);
}

void TestVideoPlaybackControlPlan::setPositionClampsToKnownDuration()
{
    const kiriview::VideoPlaybackControlPlan plan
        = kiriview::videoPlaybackSetPositionPlan(playableSnapshot(), 12000);

    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::EnsureVideoPlaybackBackendOperation>(plan, 0) != nullptr);
    const auto *setPosition = operationAt<kiriview::SetVideoPlaybackPositionOperation>(plan, 1);
    QVERIFY(setPosition != nullptr);
    QCOMPARE(setPosition->position, 10000);
    QVERIFY(plan.stateDelta.mediaEnded.has_value());
    QCOMPARE(plan.stateDelta.mediaEnded.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 10000);
}

void TestVideoPlaybackControlPlan::seekByClampsAndNoopsWhenUnchangedOrUnseekable()
{
    kiriview::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    kiriview::VideoPlaybackControlPlan plan = kiriview::videoPlaybackSeekByPlan(snapshot, 7000);

    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    QVERIFY(operationAt<kiriview::EnsureVideoPlaybackBackendOperation>(plan, 0) != nullptr);
    const auto *setPosition = operationAt<kiriview::SetVideoPlaybackPositionOperation>(plan, 1);
    QVERIFY(setPosition != nullptr);
    QCOMPARE(setPosition->position, 10000);
    QCOMPARE(plan.stateDelta.position.value(), 10000);
    QCOMPARE(kiriview::videoPlaybackClampedSeekPosition(5000, 7000, 10000, true), qint64(10000));

    snapshot.position = 0;
    QVERIFY(kiriview::videoPlaybackSeekByPlan(snapshot, -5000).isEmpty());

    snapshot.seekable = false;
    snapshot.position = 5000;
    QVERIFY(kiriview::videoPlaybackSeekByPlan(snapshot, 1000).isEmpty());
    QCOMPARE(kiriview::videoPlaybackClampedSeekPosition(5000, 1000, 10000, false), qint64(5000));
}

QTEST_GUILESS_MAIN(TestVideoPlaybackControlPlan)

#include "test_videoplaybackcontrolplan.moc"
