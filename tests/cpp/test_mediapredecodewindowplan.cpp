// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "predecode/mediapredecodeeligibility.h"
#include "predecode/mediapredecodewindowplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <vector>

namespace {
using KiriView::TestSupport::localUrl;

KiriView::MediaNavigationCandidate mediaCandidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

KiriView::PredecodePolicyInput regularPolicyInput()
{
    return KiriView::PredecodePolicyInput {
        KiriView::PredecodeDocumentKind::Regular,
        KiriView::PredecodeMomentumMode::Neutral,
        false,
        4,
    };
}

KiriView::MediaPredecodeEligibilitySnapshot eligibilitySnapshot(
    const std::vector<KiriView::MediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return KiriView::mediaPredecodeEligibilitySnapshot(candidates, currentUrl);
}
}

class TestMediaPredecodeWindowPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mediaWindowUsesVideoCursorAndQueuesOnlyImages();
    void missingCurrentCandidateYieldsEmptyWindow();
};

void TestMediaPredecodeWindowPlan::mediaWindowUsesVideoCursorAndQueuesOnlyImages()
{
    const QUrl previousImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl currentVideo = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    const QUrl nextVideo = localUrl(QStringLiteral("/media/03.mov"));
    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::mediaPredecodeWindowPlan(eligibilitySnapshot(
                                                 {
                                                     mediaCandidate(previousImage),
                                                     mediaCandidate(currentVideo),
                                                     mediaCandidate(nextImage),
                                                     mediaCandidate(nextVideo),
                                                 },
                                                 currentVideo),
            regularPolicyInput());

    QCOMPARE(windowPlan.imagePageScope, KiriView::ImagePageScopeLocation {});
    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QCOMPARE(windowPlan.urls.size(), std::size_t(2));
    QCOMPARE(windowPlan.urls.at(0), nextImage);
    QCOMPARE(windowPlan.urls.at(1), previousImage);
}

void TestMediaPredecodeWindowPlan::missingCurrentCandidateYieldsEmptyWindow()
{
    const KiriView::PredecodeWindowPlan windowPlan = KiriView::mediaPredecodeWindowPlan(
        eligibilitySnapshot(
            {
                mediaCandidate(localUrl(QStringLiteral("/media/00.png"))),
                mediaCandidate(localUrl(QStringLiteral("/media/01.mp4"))),
                mediaCandidate(localUrl(QStringLiteral("/media/02.png"))),
            },
            localUrl(QStringLiteral("/media/99.mp4"))),
        regularPolicyInput());

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.urls.empty());
}

QTEST_GUILESS_MAIN(TestMediaPredecodeWindowPlan)

#include "test_mediapredecodewindowplan.moc"
