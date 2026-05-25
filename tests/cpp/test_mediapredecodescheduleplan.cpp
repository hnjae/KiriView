// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/mediapredecodescheduleplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::MediaNavigationCandidate mediaCandidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

KiriView::DisplayedPredecodeImage displayedImage(const QUrl &url)
{
    return KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(url),
        true,
        staticTestImagePayload(testImage()),
    };
}

const std::vector<KiriView::MediaNavigationCandidate> *scheduleCandidates(
    const KiriView::MediaPredecodeSchedulePlan &plan)
{
    return KiriView::mediaPredecodeScheduleCandidates(
        KiriView::PredecodePendingSchedule { plan.context, 1 });
}

const KiriView::MediaPredecodeEligibilitySnapshot *scheduleEligibility(
    const KiriView::MediaPredecodeSchedulePlan &plan)
{
    return KiriView::mediaPredecodeScheduleEligibility(
        KiriView::PredecodePendingSchedule { plan.context, 1 });
}
}

class TestMediaPredecodeSchedulePlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void videoCursorBuildsScheduleContextAndCarriesCandidates();
    void missingCurrentCandidateKeepsUnknownPageIndex();
    void invalidCursorYieldsEmptyScheduleContext();
};

void TestMediaPredecodeSchedulePlan::videoCursorBuildsScheduleContextAndCarriesCandidates()
{
    const QUrl displayedUrl = localUrl(QStringLiteral("/media/00.png"));
    const QUrl videoUrl(QStringLiteral("file:///media/chapter/../01.mp4"));
    const QUrl normalizedVideoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));

    const KiriView::MediaPredecodeSchedulePlan plan
        = KiriView::mediaPredecodeSchedulePlan(KiriView::MediaPredecodeScheduleRequest {
            videoUrl,
            {
                mediaCandidate(displayedUrl),
                mediaCandidate(normalizedVideoUrl),
                mediaCandidate(nextUrl),
            },
            { displayedImage(displayedUrl) },
            KiriView::ImageFirstDisplayDecodeContext {},
        });

    QVERIFY(plan.shouldSchedule());
    QCOMPARE(plan.context.currentLocation.imageUrl(), normalizedVideoUrl);
    QCOMPARE(plan.context.displayedImages.size(), std::size_t(1));
    QCOMPARE(plan.context.displayedImages.front().location.imageUrl(), displayedUrl);
    QCOMPARE(plan.context.pageIndex, 1);

    const std::vector<KiriView::MediaNavigationCandidate> *candidates = scheduleCandidates(plan);
    QVERIFY(candidates != nullptr);
    QCOMPARE(candidates->size(), std::size_t(3));
    QCOMPARE(candidates->at(1).url, normalizedVideoUrl);
    const KiriView::MediaPredecodeEligibilitySnapshot *eligibility = scheduleEligibility(plan);
    QVERIFY(eligibility != nullptr);
    QCOMPARE(eligibility->mediaCandidateCount, std::size_t(3));
    QVERIFY(eligibility->currentMediaIndex.has_value());
    QCOMPARE(*eligibility->currentMediaIndex, std::size_t(1));
    QCOMPARE(eligibility->images.size(), std::size_t(2));
    QCOMPARE(eligibility->images.at(0).url, displayedUrl);
    QCOMPARE(eligibility->images.at(0).mediaIndex, std::size_t(0));
    QCOMPARE(eligibility->images.at(1).url, nextUrl);
    QCOMPARE(eligibility->images.at(1).mediaIndex, std::size_t(2));
}

void TestMediaPredecodeSchedulePlan::missingCurrentCandidateKeepsUnknownPageIndex()
{
    const QUrl videoUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const KiriView::MediaPredecodeSchedulePlan plan
        = KiriView::mediaPredecodeSchedulePlan(KiriView::MediaPredecodeScheduleRequest {
            videoUrl,
            {
                mediaCandidate(localUrl(QStringLiteral("/media/00.png"))),
                mediaCandidate(localUrl(QStringLiteral("/media/02.png"))),
            },
            {},
            KiriView::ImageFirstDisplayDecodeContext {},
        });

    QVERIFY(plan.shouldSchedule());
    QCOMPARE(plan.context.currentLocation.imageUrl(), videoUrl);
    QCOMPARE(plan.context.pageIndex, -1);
    QVERIFY(scheduleCandidates(plan) != nullptr);
    const KiriView::MediaPredecodeEligibilitySnapshot *eligibility = scheduleEligibility(plan);
    QVERIFY(eligibility != nullptr);
    QCOMPARE(eligibility->mediaCandidateCount, std::size_t(2));
    QVERIFY(!eligibility->currentMediaIndex.has_value());
}

void TestMediaPredecodeSchedulePlan::invalidCursorYieldsEmptyScheduleContext()
{
    const KiriView::MediaPredecodeSchedulePlan plan
        = KiriView::mediaPredecodeSchedulePlan(KiriView::MediaPredecodeScheduleRequest {
            QUrl(),
            { mediaCandidate(localUrl(QStringLiteral("/media/00.png"))) },
            {},
            KiriView::ImageFirstDisplayDecodeContext {},
        });

    QVERIFY(!plan.shouldSchedule());
    QVERIFY(plan.context.currentLocation.isEmpty());
    QVERIFY(scheduleCandidates(plan) == nullptr);
    QVERIFY(scheduleEligibility(plan) == nullptr);
}

QTEST_GUILESS_MAIN(TestMediaPredecodeSchedulePlan)

#include "test_mediapredecodescheduleplan.moc"
