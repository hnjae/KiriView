// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentstate.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <optional>
#include <vector>

class TestVideoDocumentState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceLoadResetsPublicPlaybackStateInOrder();
    void clearedSourceResetsPublicStateInOrder();
    void sourceLoadFailureStoresTypedFailureAndPublishesUserMessage();
    void backendFailureStoresTypedFailureAndPublishesUserMessage();
    void scalarSettersOnlyNotifyOnChangedValues();
    void zoomPercentStatePublishesKnownValuePair();
    void mutedStatePersistsAcrossSourceResets();
    void setPlayingClearsEndedState();
    void publishDeduplicatesChangesInOrder();
};

namespace {
using Change = kiriview::VideoDocumentChange;

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
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });
    const QUrl initialUrl = QUrl::fromLocalFile(QStringLiteral("/videos/old.mp4"));
    const QUrl sourceUrl(QStringLiteral("zip:///videos/archive.zip!/new.mov"));

    state.resetForSourceLoad(initialUrl);
    state.setErrorString(QStringLiteral("backend error"));
    state.setStatus(kiriview::VideoDocumentStatus::Ready);
    state.setDuration(10000);
    state.setPosition(5000);
    state.setPlaying(true);
    state.setSeekable(true);
    state.setHasVideo(true);
    state.setHasAudio(true);
    state.setVideoSize(QSize(1920, 1080));
    state.setZoomPercent(std::optional<int>(125));
    state.setMuted(true);
    batches.clear();

    state.resetForSourceLoad(sourceUrl);

    QCOMPARE(state.sourceUrl(), sourceUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("new.mov"));
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Loading);
    QCOMPARE(state.duration(), 0);
    QCOMPARE(state.position(), 0);
    QVERIFY(!state.playing());
    QVERIFY(!state.seekable());
    QVERIFY(!state.hasVideo());
    QVERIFY(!state.hasAudio());
    QCOMPARE(state.videoSize(), QSize());
    QVERIFY(!state.zoomPercentKnown());
    QCOMPARE(state.zoomPercent(), 0);
    QVERIFY(state.muted());
    compareChanges(batches.front(),
        { Change::SourceUrl, Change::WindowTitleFileName, Change::ErrorString, Change::Status,
            Change::Duration, Change::Position, Change::Playing, Change::Seekable, Change::HasVideo,
            Change::HasAudio, Change::VideoSize, Change::ZoomPercentKnown, Change::ZoomPercent,
            Change::EmbeddedMetadata });
}

void TestVideoDocumentState::clearedSourceResetsPublicStateInOrder()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.resetForSourceLoad(QUrl::fromLocalFile(QStringLiteral("/videos/clip.mp4")));
    state.setErrorString(QStringLiteral("backend error"));
    state.setStatus(kiriview::VideoDocumentStatus::Ready);
    state.setDuration(10000);
    state.setPosition(5000);
    state.setPlaying(true);
    state.setSeekable(true);
    state.setHasVideo(true);
    state.setHasAudio(true);
    state.setVideoSize(QSize(1920, 1080));
    state.setZoomPercent(std::optional<int>(125));
    state.setMuted(true);
    batches.clear();

    state.resetForClearedSource();

    QCOMPARE(state.sourceUrl(), QUrl());
    QCOMPARE(state.windowTitleFileName(), QString());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Null);
    QCOMPARE(state.duration(), 0);
    QCOMPARE(state.position(), 0);
    QVERIFY(!state.playing());
    QVERIFY(!state.seekable());
    QVERIFY(!state.hasVideo());
    QVERIFY(!state.hasAudio());
    QCOMPARE(state.videoSize(), QSize());
    QVERIFY(!state.zoomPercentKnown());
    QCOMPARE(state.zoomPercent(), 0);
    QVERIFY(state.muted());
    compareChanges(batches.front(),
        { Change::SourceUrl, Change::Status, Change::ErrorString, Change::WindowTitleFileName,
            Change::Duration, Change::Position, Change::Playing, Change::Seekable, Change::HasVideo,
            Change::HasAudio, Change::VideoSize, Change::ZoomPercentKnown, Change::ZoomPercent,
            Change::EmbeddedMetadata });
}

void TestVideoDocumentState::sourceLoadFailureStoresTypedFailureAndPublishesUserMessage()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });
    const QUrl sourceUrl(QStringLiteral("zip:///videos/archive.zip!/clip.mp4"));
    const QString userMessage = QStringLiteral("Could not open the selected video.");
    const QString diagnosticDetail = QStringLiteral("resolver rejected archive entry");

    state.resetForSourceLoad(sourceUrl);
    batches.clear();
    state.setSourceLoadFailure(kiriview::VideoSourceLoadFailure {
        sourceUrl,
        kiriview::VideoSourceLoadFailureKind::PlaybackUrlResolution,
        userMessage,
        diagnosticDetail,
        kiriview::VideoSourceLoadFailureSeverity::Error,
        false,
    });

    QVERIFY(state.sourceLoadFailure().has_value());
    QCOMPARE(state.sourceLoadFailure()->sourceUrl, sourceUrl);
    QVERIFY(state.sourceLoadFailure()->kind
        == kiriview::VideoSourceLoadFailureKind::PlaybackUrlResolution);
    QCOMPARE(state.sourceLoadFailure()->userMessage, userMessage);
    QCOMPARE(state.sourceLoadFailure()->diagnosticDetail, diagnosticDetail);
    QVERIFY(state.sourceLoadFailure()->severity == kiriview::VideoSourceLoadFailureSeverity::Error);
    QVERIFY(!state.sourceLoadFailure()->retryable);
    QCOMPARE(state.errorString(), userMessage);
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Error);
    compareChanges(batches.front(), { Change::ErrorString, Change::Status });

    batches.clear();
    state.resetForSourceLoad(QUrl::fromLocalFile(QStringLiteral("/videos/next.mp4")));

    QVERIFY(!state.sourceLoadFailure().has_value());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Loading);
}

void TestVideoDocumentState::backendFailureStoresTypedFailureAndPublishesUserMessage()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/videos/clip.mp4"));
    const QString backendError = QStringLiteral("Qt Multimedia backend rejected stream");

    state.resetForSourceLoad(sourceUrl);
    batches.clear();
    state.setBackendFailure(kiriview::VideoBackendFailure {
        sourceUrl,
        kiriview::VideoBackendFailureKind::Playback,
        backendError,
        backendError,
        kiriview::VideoBackendFailureSeverity::Error,
        false,
    });

    QVERIFY(state.backendFailure().has_value());
    QCOMPARE(state.backendFailure()->sourceUrl, sourceUrl);
    QVERIFY(state.backendFailure()->kind == kiriview::VideoBackendFailureKind::Playback);
    QCOMPARE(state.backendFailure()->userMessage, backendError);
    QCOMPARE(state.backendFailure()->diagnosticDetail, backendError);
    QVERIFY(state.backendFailure()->severity == kiriview::VideoBackendFailureSeverity::Error);
    QVERIFY(!state.backendFailure()->retryable);
    QVERIFY(!state.sourceLoadFailure().has_value());
    QCOMPARE(state.errorString(), backendError);
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Error);
    compareChanges(batches.front(), { Change::ErrorString, Change::Status });

    batches.clear();
    state.setStatusAndError(kiriview::VideoDocumentStatus::Ready);

    QVERIFY(!state.backendFailure().has_value());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Ready);
    compareChanges(batches.front(), { Change::ErrorString, Change::Status });

    state.setBackendFailure(kiriview::VideoBackendFailure {
        sourceUrl,
        kiriview::VideoBackendFailureKind::Playback,
        backendError,
        backendError,
        kiriview::VideoBackendFailureSeverity::Error,
        false,
    });
    state.resetForSourceLoad(QUrl::fromLocalFile(QStringLiteral("/videos/next.mp4")));

    QVERIFY(!state.backendFailure().has_value());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(state.status(), kiriview::VideoDocumentStatus::Loading);
}

void TestVideoDocumentState::scalarSettersOnlyNotifyOnChangedValues()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.setDuration(-10);
    state.setPosition(-10);
    state.setVideoSize(QSize(-1, 1080));
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
    state.setVideoSize(QSize(1920, 1080));
    state.setVideoSize(QSize(1920, 1080));
    state.setMuted(true);
    state.setMuted(true);

    QCOMPARE(state.duration(), 5000);
    QCOMPARE(state.position(), 2500);
    QVERIFY(state.seekable());
    QVERIFY(state.hasVideo());
    QVERIFY(state.hasAudio());
    QCOMPARE(state.videoSize(), QSize(1920, 1080));
    QVERIFY(state.muted());
    compareChanges(flatten(batches),
        { Change::Duration, Change::Position, Change::Seekable, Change::HasVideo, Change::HasAudio,
            Change::VideoSize, Change::Muted });
}

void TestVideoDocumentState::zoomPercentStatePublishesKnownValuePair()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.setZoomPercent(std::optional<int>(150));

    QVERIFY(state.zoomPercentKnown());
    QCOMPARE(state.zoomPercent(), 150);
    compareChanges(batches.front(), { Change::ZoomPercent, Change::ZoomPercentKnown });

    batches.clear();
    state.setZoomPercent(std::optional<int>(150));
    QVERIFY(batches.empty());

    state.setZoomPercent(std::nullopt);

    QVERIFY(!state.zoomPercentKnown());
    QCOMPARE(state.zoomPercent(), 0);
    compareChanges(batches.front(), { Change::ZoomPercentKnown, Change::ZoomPercent });
}

void TestVideoDocumentState::mutedStatePersistsAcrossSourceResets()
{
    std::vector<std::vector<Change>> batches;
    kiriview::VideoDocumentState state(
        [&batches](const std::vector<Change> &changes) { batches.push_back(changes); });

    state.setMuted(true);
    QCOMPARE(batches.size(), std::size_t(1));
    compareChanges(batches.front(), { Change::Muted });

    batches.clear();
    state.resetForSourceLoad(QUrl::fromLocalFile(QStringLiteral("/videos/clip.mp4")));
    QVERIFY(state.muted());
    QVERIFY(std::find(batches.front().cbegin(), batches.front().cend(), Change::Muted)
        == batches.front().cend());

    batches.clear();
    state.resetForClearedSource();
    QVERIFY(state.muted());
    QVERIFY(std::find(batches.front().cbegin(), batches.front().cend(), Change::Muted)
        == batches.front().cend());
}

void TestVideoDocumentState::setPlayingClearsEndedState()
{
    kiriview::VideoDocumentState state;

    state.setMediaEnded(true);
    QVERIFY(state.mediaEnded());

    state.setPlaying(true);
    QVERIFY(state.playing());
    QVERIFY(!state.mediaEnded());
}

void TestVideoDocumentState::publishDeduplicatesChangesInOrder()
{
    std::vector<Change> published;
    kiriview::VideoDocumentState state(
        [&published](const std::vector<Change> &changes) { published = changes; });

    state.publish({ Change::Position, Change::Status, Change::Position, Change::Playing });

    compareChanges(published, { Change::Position, Change::Status, Change::Playing });
}

QTEST_GUILESS_MAIN(TestVideoDocumentState)

#include "test_videodocumentstate.moc"
