// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "candidate_test_support.h"
#include "imagecontainer.h"
#include "predecode/predecodewindowplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::localUrl;

std::vector<KiriView::ImageNavigationCandidate> imageCandidates(int count)
{
    std::vector<KiriView::ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(imageCandidate(indexedImageUrl(index)));
    }
    return candidates;
}
}

class TestPredecodeWindowPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void regularImagePlansCandidateContextAndNeutralWindow();
    void powerSaverSuppressesCandidateLoading();
    void missingCandidateContextStillCarriesFallbackWindow();
    void directoryDocumentUsesDocumentParallelLimit();
    void archiveWindowPreservesArchiveDocumentContext();
    void missingCurrentCandidateYieldsEmptyWindow();
};

void TestPredecodeWindowPlan::regularImagePlansCandidateContextAndNeutralWindow()
{
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(5)),
            KiriView::PredecodeMomentumMode::Neutral,
            false,
            4,
        });

    QVERIFY(candidateListPlan.shouldLoadCandidates());
    QVERIFY(candidateListPlan.archiveDocument.isEmpty());
    QVERIFY(candidateListPlan.candidateContext.has_value());
    QCOMPARE(candidateListPlan.candidateContext->currentUrl(), indexedImageUrl(5));

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanForCandidates(candidateListPlan, imageCandidates(15));

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QCOMPARE(windowPlan.urls.size(), std::size_t(4));
    QCOMPARE(windowPlan.urls.at(0), indexedImageUrl(5));
    QCOMPARE(windowPlan.urls.at(1), indexedImageUrl(6));
    QCOMPARE(windowPlan.urls.at(2), indexedImageUrl(4));
    QCOMPARE(windowPlan.urls.at(3), indexedImageUrl(7));
}

void TestPredecodeWindowPlan::powerSaverSuppressesCandidateLoading()
{
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(5)),
            KiriView::PredecodeMomentumMode::Neutral,
            true,
            4,
        });

    QVERIFY(!candidateListPlan.shouldLoadCandidates());
    QVERIFY(candidateListPlan.candidateContext.has_value());

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanWithoutCandidates(candidateListPlan);
    QCOMPARE(windowPlan.parallelLimit, std::size_t(0));
    QVERIFY(windowPlan.urls.empty());
}

void TestPredecodeWindowPlan::missingCandidateContextStillCarriesFallbackWindow()
{
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromUrl(QUrl()),
            KiriView::PredecodeMomentumMode::Neutral,
            false,
            4,
        });

    QVERIFY(!candidateListPlan.shouldLoadCandidates());
    QVERIFY(!candidateListPlan.candidateContext.has_value());

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanWithoutCandidates(candidateListPlan);
    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.archiveDocument.isEmpty());
    QVERIFY(windowPlan.urls.empty());
}

void TestPredecodeWindowPlan::directoryDocumentUsesDocumentParallelLimit()
{
    const KiriView::ArchiveDocumentLocation directoryDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            imagesDirectoryUrl(), imagesDirectoryUrl(), KiriView::ArchiveDocumentKind::Directory);
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromArchiveDocument(
                indexedImageUrl(5), directoryDocument),
            KiriView::PredecodeMomentumMode::Neutral,
            false,
            4,
        });

    QVERIFY(candidateListPlan.shouldLoadCandidates());
    QCOMPARE(candidateListPlan.archiveDocument, directoryDocument);

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanForCandidates(candidateListPlan, imageCandidates(15));

    QCOMPARE(windowPlan.parallelLimit, std::size_t(2));
    QVERIFY(windowPlan.urls.size() >= 2);
    QCOMPARE(windowPlan.urls.at(0), indexedImageUrl(5));
    QCOMPARE(windowPlan.urls.at(1), indexedImageUrl(6));
}

void TestPredecodeWindowPlan::archiveWindowPreservesArchiveDocumentContext()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const QUrl displayedUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, *archiveDocument),
            KiriView::PredecodeMomentumMode::Neutral,
            false,
            4,
        });

    QVERIFY(candidateListPlan.shouldLoadCandidates());
    QCOMPARE(candidateListPlan.archiveDocument, *archiveDocument);

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanForCandidates(candidateListPlan,
            {
                imageCandidate(displayedUrl),
                imageCandidate(nextUrl),
            });

    QCOMPARE(windowPlan.archiveDocument, *archiveDocument);
    QCOMPARE(windowPlan.parallelLimit, std::size_t(2));
    QCOMPARE(windowPlan.urls.size(), std::size_t(2));
    QCOMPARE(windowPlan.urls.at(0), displayedUrl);
    QCOMPARE(windowPlan.urls.at(1), nextUrl);
}

void TestPredecodeWindowPlan::missingCurrentCandidateYieldsEmptyWindow()
{
    const KiriView::PredecodeCandidateListPlan candidateListPlan
        = KiriView::predecodeCandidateListPlan(KiriView::PredecodeWindowPlanRequest {
            KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(5)),
            KiriView::PredecodeMomentumMode::Neutral,
            false,
            4,
        });

    const KiriView::PredecodeWindowPlan windowPlan
        = KiriView::predecodeWindowPlanForCandidates(candidateListPlan,
            {
                imageCandidate(indexedImageUrl(0)),
                imageCandidate(indexedImageUrl(1)),
                imageCandidate(indexedImageUrl(2)),
            });

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.urls.empty());
}

QTEST_GUILESS_MAIN(TestPredecodeWindowPlan)

#include "test_predecodewindowplan.moc"
