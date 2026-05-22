// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

class TestVideoDocumentState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadResetsPublicPlaybackStateInOrder();
    void clearedSourceResetsPublicStateInOrder();
    void scalarSettersOnlyNotifyOnChangedValues();
    void setPlayingClearsEndedState();
    void publishDeduplicatesChangesInOrder();
};

namespace {
using Change = KiriView::VideoDocumentChange;

std::vector<Change> flatten(const std::vector<std::vector<Change>> &batches)
{
    std::vector<Change> changes;
    for (const std::vector<Change> &batch : batches) {
        changes.insert(changes.end(), batch.cbegin(), batch.cend());
    }
    return changes;
}

void compareChanges(const std::vector<Change> &actual, const std::vector<Change> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

void TestVideoDocumentState::sourceLoadResetsPublicPlaybackStateInOrder()
{
    std::vector<std::vector<Change>> batches;
    KiriView::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });
    const QUrl initialUrl = QUrl::fromLocalFile(QStringLiteral("/videos/old.mp4"));
    const QUrl sourceUrl(QStringLiteral("zip:///videos/archive.zip!/new.mov"));

    state.resetForSourceLoad(initialUrl);
    state.setErrorString(QStringLiteral("backend error"));
    state.setStatus(KiriView::VideoDocumentStatus::Ready);
    state.setDuration(10000);
    state.setPosition(5000);
    state.setPlaying(true);
    state.setSeekable(true);
    state.setHasVideo(true);
    state.setHasAudio(true);
    batches.clear();

    state.resetForSourceLoad(sourceUrl);

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("new.mov"));
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), KiriView::VideoDocumentStatus::Loading);
    QCOMPARE(state.duration(), 0);
    QCOMPARE(state.position(), 0);
    QVERIFY(!state.playing());
    QVERIFY(!state.seekable());
    QVERIFY(!state.hasVideo());
    QVERIFY(!state.hasAudio());
    compareChanges(batches.front(),
        { Change::SourceUrl, Change::WindowTitleFileName, Change::ErrorString, Change::Status,
            Change::Duration, Change::Position, Change::Playing, Change::Seekable, Change::HasVideo,
            Change::HasAudio });
}

void TestVideoDocumentState::clearedSourceResetsPublicStateInOrder()
{
    std::vector<std::vector<Change>> batches;
    KiriView::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.resetForSourceLoad(QUrl::fromLocalFile(QStringLiteral("/videos/clip.mp4")));
    state.setErrorString(QStringLiteral("backend error"));
    state.setStatus(KiriView::VideoDocumentStatus::Ready);
    state.setDuration(10000);
    state.setPosition(5000);
    state.setPlaying(true);
    state.setSeekable(true);
    state.setHasVideo(true);
    state.setHasAudio(true);
    batches.clear();

    state.resetForClearedSource();

    QCOMPARE(state.sourceUrl(), QUrl());
    QCOMPARE(state.windowTitleFileName(), QString());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), KiriView::VideoDocumentStatus::Null);
    QCOMPARE(state.duration(), 0);
    QCOMPARE(state.position(), 0);
    QVERIFY(!state.playing());
    QVERIFY(!state.seekable());
    QVERIFY(!state.hasVideo());
    QVERIFY(!state.hasAudio());
    compareChanges(batches.front(),
        { Change::SourceUrl, Change::Status, Change::ErrorString, Change::WindowTitleFileName,
            Change::Duration, Change::Position, Change::Playing, Change::Seekable, Change::HasVideo,
            Change::HasAudio });
}

void TestVideoDocumentState::scalarSettersOnlyNotifyOnChangedValues()
{
    std::vector<std::vector<Change>> batches;
    KiriView::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.setDuration(-10);
    state.setPosition(-10);
    QVERIFY(batches.empty());

    state.setDuration(5000);
    state.setDuration(5000);
    state.setPosition(2500);
    state.setPosition(2500);
    state.setSeekable(true);
    state.setSeekable(true);
    state.setHasVideo(true);
    state.setHasVideo(true);
    state.setHasAudio(true);
    state.setHasAudio(true);

    QCOMPARE(state.duration(), 5000);
    QCOMPARE(state.position(), 2500);
    QVERIFY(state.seekable());
    QVERIFY(state.hasVideo());
    QVERIFY(state.hasAudio());
    compareChanges(flatten(batches),
        { Change::Duration, Change::Position, Change::Seekable, Change::HasVideo,
            Change::HasAudio });
}

void TestVideoDocumentState::setPlayingClearsEndedState()
{
    KiriView::VideoDocumentState state;

    state.setEnded(true);
    QVERIFY(state.ended());

    state.setPlaying(true);
    QVERIFY(state.playing());
    QVERIFY(!state.ended());
}

void TestVideoDocumentState::publishDeduplicatesChangesInOrder()
{
    std::vector<Change> published;
    KiriView::VideoDocumentState state(
        [&published](const std::vector<Change> &changes) { published = changes; });

    state.publish({ Change::Position, Change::Status, Change::Position, Change::Playing });

    compareChanges(published, { Change::Position, Change::Status, Change::Playing });
}

QTEST_GUILESS_MAIN(TestVideoDocumentState)

#include "test_videodocumentstate.moc"
