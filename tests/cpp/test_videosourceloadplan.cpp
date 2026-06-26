// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadplan.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <type_traits>
#include <variant>

class TestVideoSourceLoadPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void clearPlanClearsPlaybackBeforePublicState();
    void startPlanClearsPlaybackBeforePublishingSourceLoad();
    void readyPlanCarriesSourceAndPlaybackUrls();
    void failurePlanCarriesSourceAndError();
};

namespace {
template <typename Operation>
const Operation& operationAt(const kiriview::VideoSourceLoadPlan& plan, std::size_t index)
{
    return std::get<Operation>(plan.at(index));
}
}

void TestVideoSourceLoadPlan::clearPlanClearsPlaybackBeforePublicState()
{
    const kiriview::VideoSourceLoadPlan plan = kiriview::videoSourceLoadClearPlan();

    QCOMPARE(plan.size(), std::size_t(2));
    QVERIFY(std::holds_alternative<kiriview::ClearVideoPlaybackSourceOperation>(plan.at(0)));
    QVERIFY(std::holds_alternative<kiriview::ResetClearedVideoSourceOperation>(plan.at(1)));
}

void TestVideoSourceLoadPlan::startPlanClearsPlaybackBeforePublishingSourceLoad()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const kiriview::VideoSourceLoadPlan plan = kiriview::videoSourceLoadStartPlan(sourceUrl);

    QCOMPARE(plan.size(), std::size_t(2));
    QVERIFY(std::holds_alternative<kiriview::ClearVideoPlaybackSourceOperation>(plan.at(0)));
    QCOMPARE(operationAt<kiriview::ResetVideoSourceLoadOperation>(plan, 1).sourceUrl, sourceUrl);
}

void TestVideoSourceLoadPlan::readyPlanCarriesSourceAndPlaybackUrls()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QUrl playbackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));
    const kiriview::VideoSourceLoadPlan plan
        = kiriview::videoSourceLoadReadyPlan(sourceUrl, playbackUrl);

    QCOMPARE(plan.size(), std::size_t(1));
    const kiriview::ApplyVideoPlaybackUrlOperation& operation
        = operationAt<kiriview::ApplyVideoPlaybackUrlOperation>(plan, 0);
    QCOMPARE(operation.sourceUrl, sourceUrl);
    QCOMPARE(operation.playbackUrl, playbackUrl);
}

void TestVideoSourceLoadPlan::failurePlanCarriesSourceAndError()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QString errorString = QStringLiteral("resolution failed");
    const QString userMessage = QStringLiteral("Could not open the selected video.");
    const kiriview::VideoSourceLoadPlan plan
        = kiriview::videoSourceLoadFailurePlan(sourceUrl, errorString);

    QCOMPARE(plan.size(), std::size_t(1));
    const kiriview::PublishVideoSourceLoadFailureOperation& operation
        = operationAt<kiriview::PublishVideoSourceLoadFailureOperation>(plan, 0);
    QCOMPARE(operation.failure.sourceUrl, sourceUrl);
    QVERIFY(operation.failure.kind == kiriview::VideoSourceLoadFailureKind::PlaybackUrlResolution);
    QCOMPARE(operation.failure.userMessage, userMessage);
    QCOMPARE(operation.failure.diagnosticDetail, errorString);
    QVERIFY(operation.failure.severity == kiriview::VideoSourceLoadFailureSeverity::Error);
    QVERIFY(!operation.failure.retryable);
}

QTEST_GUILESS_MAIN(TestVideoSourceLoadPlan)

#include "test_videosourceloadplan.moc"
