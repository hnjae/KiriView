// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackcontrolplan.h"

#include <QObject>
#include <QTest>

class TestVideoPlaybackControlPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void playNoopsWithoutSource();
    void playRestartsEndedSeekableMediaBeforePlaying();
    void pauseRequiresExistingBackend();
    void toggleDispatchesFromCurrentPlaybackState();
    void stopDoesNotCreateBackend();
    void setPositionClampsToKnownDuration();
    void seekByClampsAndNoopsWhenUnchangedOrUnseekable();
};

namespace {
using Kind = KiriView::VideoPlaybackBackendOperationKind;

KiriView::VideoPlaybackControlSnapshot playableSnapshot()
{
    return KiriView::VideoPlaybackControlSnapshot {
        false,
        true,
        false,
        false,
        true,
        5000,
        10000,
    };
}

void compareOperation(
    const KiriView::VideoPlaybackBackendOperation &operation, Kind kind, qint64 position = 0)
{
    QCOMPARE(operation.kind, kind);
    QCOMPARE(operation.position, position);
}
}

void TestVideoPlaybackControlPlan::playNoopsWithoutSource()
{
    const KiriView::VideoPlaybackControlPlan plan
        = KiriView::videoPlaybackPlayPlan(KiriView::VideoPlaybackControlSnapshot {});

    QVERIFY(plan.isEmpty());
}

void TestVideoPlaybackControlPlan::playRestartsEndedSeekableMediaBeforePlaying()
{
    KiriView::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.ended = true;
    snapshot.position = 10000;

    const KiriView::VideoPlaybackControlPlan plan = KiriView::videoPlaybackPlayPlan(snapshot);

    QCOMPARE(plan.backendOperations.size(), std::size_t(3));
    compareOperation(plan.backendOperations.at(0), Kind::EnsureBackend);
    compareOperation(plan.backendOperations.at(1), Kind::SetPosition, 0);
    compareOperation(plan.backendOperations.at(2), Kind::Play);
    QVERIFY(plan.stateDelta.ended.has_value());
    QCOMPARE(plan.stateDelta.ended.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 0);
    QVERIFY(!plan.stateDelta.playing.has_value());
}

void TestVideoPlaybackControlPlan::pauseRequiresExistingBackend()
{
    KiriView::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.mediaBackendAvailable = false;
    QVERIFY(KiriView::videoPlaybackPausePlan(snapshot).isEmpty());

    snapshot.mediaBackendAvailable = true;
    const KiriView::VideoPlaybackControlPlan plan = KiriView::videoPlaybackPausePlan(snapshot);

    QCOMPARE(plan.backendOperations.size(), std::size_t(1));
    compareOperation(plan.backendOperations.front(), Kind::Pause);
    QVERIFY(!plan.stateDelta.playing.has_value());
}

void TestVideoPlaybackControlPlan::toggleDispatchesFromCurrentPlaybackState()
{
    KiriView::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.playing = true;

    KiriView::VideoPlaybackControlPlan plan = KiriView::videoPlaybackTogglePlan(snapshot);
    QCOMPARE(plan.backendOperations.size(), std::size_t(1));
    compareOperation(plan.backendOperations.front(), Kind::Pause);

    snapshot.playing = false;
    plan = KiriView::videoPlaybackTogglePlan(snapshot);
    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    compareOperation(plan.backendOperations.at(0), Kind::EnsureBackend);
    compareOperation(plan.backendOperations.at(1), Kind::Play);
}

void TestVideoPlaybackControlPlan::stopDoesNotCreateBackend()
{
    KiriView::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    snapshot.mediaBackendAvailable = false;
    snapshot.playing = true;
    snapshot.position = 4000;

    const KiriView::VideoPlaybackControlPlan plan = KiriView::videoPlaybackStopPlan(snapshot);

    QVERIFY(plan.backendOperations.empty());
    QVERIFY(plan.stateDelta.ended.has_value());
    QCOMPARE(plan.stateDelta.ended.value(), false);
    QVERIFY(plan.stateDelta.playing.has_value());
    QCOMPARE(plan.stateDelta.playing.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 0);
}

void TestVideoPlaybackControlPlan::setPositionClampsToKnownDuration()
{
    const KiriView::VideoPlaybackControlPlan plan
        = KiriView::videoPlaybackSetPositionPlan(playableSnapshot(), 12000);

    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    compareOperation(plan.backendOperations.at(0), Kind::EnsureBackend);
    compareOperation(plan.backendOperations.at(1), Kind::SetPosition, 10000);
    QVERIFY(plan.stateDelta.ended.has_value());
    QCOMPARE(plan.stateDelta.ended.value(), false);
    QVERIFY(plan.stateDelta.position.has_value());
    QCOMPARE(plan.stateDelta.position.value(), 10000);
}

void TestVideoPlaybackControlPlan::seekByClampsAndNoopsWhenUnchangedOrUnseekable()
{
    KiriView::VideoPlaybackControlSnapshot snapshot = playableSnapshot();
    KiriView::VideoPlaybackControlPlan plan = KiriView::videoPlaybackSeekByPlan(snapshot, 7000);

    QCOMPARE(plan.backendOperations.size(), std::size_t(2));
    compareOperation(plan.backendOperations.at(0), Kind::EnsureBackend);
    compareOperation(plan.backendOperations.at(1), Kind::SetPosition, 10000);
    QCOMPARE(plan.stateDelta.position.value(), 10000);
    QCOMPARE(KiriView::videoPlaybackClampedSeekPosition(5000, 7000, 10000, true), qint64(10000));

    snapshot.position = 0;
    QVERIFY(KiriView::videoPlaybackSeekByPlan(snapshot, -5000).isEmpty());

    snapshot.seekable = false;
    snapshot.position = 5000;
    QVERIFY(KiriView::videoPlaybackSeekByPlan(snapshot, 1000).isEmpty());
    QCOMPARE(KiriView::videoPlaybackClampedSeekPosition(5000, 1000, 10000, false), qint64(5000));
}

QTEST_GUILESS_MAIN(TestVideoPlaybackControlPlan)

#include "test_videoplaybackcontrolplan.moc"
