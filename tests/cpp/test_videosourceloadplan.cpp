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
const Operation &operationAt(const KiriView::VideoSourceLoadPlan &plan, std::size_t index)
{
    return std::get<Operation>(plan.at(index));
}
}

void TestVideoSourceLoadPlan::clearPlanClearsPlaybackBeforePublicState()
{
    const KiriView::VideoSourceLoadPlan plan = KiriView::videoSourceLoadClearPlan();

    QCOMPARE(plan.size(), std::size_t(2));
    QVERIFY(std::holds_alternative<KiriView::ClearVideoPlaybackSourceOperation>(plan.at(0)));
    QVERIFY(std::holds_alternative<KiriView::ResetClearedVideoSourceOperation>(plan.at(1)));
}

void TestVideoSourceLoadPlan::startPlanClearsPlaybackBeforePublishingSourceLoad()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const KiriView::VideoSourceLoadPlan plan = KiriView::videoSourceLoadStartPlan(sourceUrl);

    QCOMPARE(plan.size(), std::size_t(2));
    QVERIFY(std::holds_alternative<KiriView::ClearVideoPlaybackSourceOperation>(plan.at(0)));
    QCOMPARE(operationAt<KiriView::ResetVideoSourceLoadOperation>(plan, 1).sourceUrl, sourceUrl);
}

void TestVideoSourceLoadPlan::readyPlanCarriesSourceAndPlaybackUrls()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QUrl playbackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));
    const KiriView::VideoSourceLoadPlan plan
        = KiriView::videoSourceLoadReadyPlan(sourceUrl, playbackUrl);

    QCOMPARE(plan.size(), std::size_t(1));
    const KiriView::ApplyVideoPlaybackUrlOperation &operation
        = operationAt<KiriView::ApplyVideoPlaybackUrlOperation>(plan, 0);
    QCOMPARE(operation.sourceUrl, sourceUrl);
    QCOMPARE(operation.playbackUrl, playbackUrl);
}

void TestVideoSourceLoadPlan::failurePlanCarriesSourceAndError()
{
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QString errorString = QStringLiteral("resolution failed");
    const KiriView::VideoSourceLoadPlan plan
        = KiriView::videoSourceLoadFailurePlan(sourceUrl, errorString);

    QCOMPARE(plan.size(), std::size_t(1));
    const KiriView::PublishVideoSourceLoadFailureOperation &operation
        = operationAt<KiriView::PublishVideoSourceLoadFailureOperation>(plan, 0);
    QCOMPARE(operation.sourceUrl, sourceUrl);
    QCOMPARE(operation.errorString, errorString);
}

QTEST_GUILESS_MAIN(TestVideoSourceLoadPlan)

#include "test_videosourceloadplan.moc"
