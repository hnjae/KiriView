// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
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
}

class TestMediaPredecodeWindowPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stillImageEligibilityUsesCandidateNameAndUrlIdentity();
    void mediaWindowUsesVideoCursorAndQueuesOnlyImages();
    void missingCurrentCandidateYieldsEmptyWindow();
};

void TestMediaPredecodeWindowPlan::stillImageEligibilityUsesCandidateNameAndUrlIdentity()
{
    QVERIFY(KiriView::mediaPredecodeStillImageEligible(KiriView::MediaNavigationCandidate {
        localUrl(QStringLiteral("/media/blob.bin")),
        QStringLiteral("cover.png"),
    }));
    QVERIFY(KiriView::mediaPredecodeStillImageEligible(KiriView::MediaNavigationCandidate {
        localUrl(QStringLiteral("/media/photo.avif")),
        QString(),
    }));
    QVERIFY(KiriView::mediaPredecodeStillImageEligible(KiriView::MediaNavigationCandidate {
        QUrl(QStringLiteral("file:///media/download?name=cover.webp")),
        QString(),
    }));
    QVERIFY(!KiriView::mediaPredecodeStillImageEligible(
        mediaCandidate(localUrl(QStringLiteral("/media/clip.mp4")))));
}

void TestMediaPredecodeWindowPlan::mediaWindowUsesVideoCursorAndQueuesOnlyImages()
{
    const QUrl previousImage = localUrl(QStringLiteral("/media/00.png"));
    const QUrl currentVideo = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextImage = localUrl(QStringLiteral("/media/02.png"));
    const QUrl nextVideo = localUrl(QStringLiteral("/media/03.mov"));
    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::mediaPredecodeWindowPlan(currentVideo,
            {
                mediaCandidate(previousImage),
                mediaCandidate(currentVideo),
                mediaCandidate(nextImage),
                mediaCandidate(nextVideo),
            },
            regularPolicyInput());

    QCOMPARE(windowPlan.archiveDocument, KiriView::ArchiveDocumentLocation {});
    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QCOMPARE(windowPlan.urls.size(), std::size_t(2));
    QCOMPARE(windowPlan.urls.at(0), nextImage);
    QCOMPARE(windowPlan.urls.at(1), previousImage);
}

void TestMediaPredecodeWindowPlan::missingCurrentCandidateYieldsEmptyWindow()
{
    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::mediaPredecodeWindowPlan(localUrl(QStringLiteral("/media/99.mp4")),
            {
                mediaCandidate(localUrl(QStringLiteral("/media/00.png"))),
                mediaCandidate(localUrl(QStringLiteral("/media/01.mp4"))),
                mediaCandidate(localUrl(QStringLiteral("/media/02.png"))),
            },
            regularPolicyInput());

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.urls.empty());
}

QTEST_GUILESS_MAIN(TestMediaPredecodeWindowPlan)

#include "test_mediapredecodewindowplan.moc"
