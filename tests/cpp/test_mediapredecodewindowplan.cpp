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
using kiriview::TestSupport::localUrl;

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::PredecodePolicyInput regularPolicyInput()
{
    return kiriview::PredecodePolicyInput {
        kiriview::directMediaPredecodeSourceProfile(),
        kiriview::PredecodeMomentumMode::Neutral,
        false,
    };
}

kiriview::MediaPredecodeEligibilitySnapshot eligibilitySnapshot(
    const std::vector<kiriview::DirectMediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return kiriview::mediaPredecodeEligibilitySnapshot(candidates, currentUrl);
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
    const QUrl secondPreviousImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl previousImage = localUrl(QStringLiteral("/media/01.png"));
    const QUrl currentVideo = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/03.png"));
    const QUrl secondNextImage = localUrl(QStringLiteral("/media/04.png"));
    const QUrl nextVideo = localUrl(QStringLiteral("/media/05.mov"));
    const kiriview::PredecodeWindowPlan windowPlan = kiriview::mediaPredecodeWindowPlan(
        eligibilitySnapshot(
            {
                directMediaNavigationCandidate(secondPreviousImage),
                directMediaNavigationCandidate(previousImage),
                directMediaNavigationCandidate(currentVideo),
                directMediaNavigationCandidate(nextImage),
                directMediaNavigationCandidate(secondNextImage),
                directMediaNavigationCandidate(nextVideo),
            },
            currentVideo),
        regularPolicyInput());

    QCOMPARE(windowPlan.openedCollectionScope, kiriview::OpenedCollectionScopeLocation {});
    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QCOMPARE(windowPlan.urls.size(), std::size_t(4));
    QCOMPARE(windowPlan.urls.at(0), nextImage);
    QCOMPARE(windowPlan.urls.at(1), previousImage);
    QCOMPARE(windowPlan.urls.at(2), secondNextImage);
    QCOMPARE(windowPlan.urls.at(3), secondPreviousImage);
}

void TestMediaPredecodeWindowPlan::missingCurrentCandidateYieldsEmptyWindow()
{
    const kiriview::PredecodeWindowPlan windowPlan = kiriview::mediaPredecodeWindowPlan(
        eligibilitySnapshot(
            {
                directMediaNavigationCandidate(localUrl(QStringLiteral("/media/00.png"))),
                directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.mp4"))),
                directMediaNavigationCandidate(localUrl(QStringLiteral("/media/02.png"))),
            },
            localUrl(QStringLiteral("/media/99.mp4"))),
        regularPolicyInput());

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.urls.empty());
}

QTEST_GUILESS_MAIN(TestMediaPredecodeWindowPlan)

#include "test_mediapredecodewindowplan.moc"
