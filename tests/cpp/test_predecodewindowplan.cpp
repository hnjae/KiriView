// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"
#include "predecode/predecodewindowplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::imagesDirectoryUrl;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::videoCandidate;

std::vector<kiriview::ImageDocumentPageCandidate> imageDocumentPageCandidates(int count)
{
    std::vector<kiriview::ImageDocumentPageCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(imageDocumentPageCandidate(indexedImageUrl(index)));
    }
    return candidates;
}

kiriview::PredecodePolicyInput policyInputForLocation(
    const kiriview::DisplayedImageLocation &location,
    kiriview::PredecodeMomentumMode momentumMode = kiriview::PredecodeMomentumMode::Neutral,
    bool powerSaverEnabled = false, int idealThreadCount = 4)
{
    return kiriview::PredecodePolicyInput {
        kiriview::predecodeSourceProfileForOpenedCollectionScope(
            location.openedCollectionScope(), idealThreadCount),
        momentumMode,
        powerSaverEnabled,
    };
}

kiriview::PredecodeWindowStartPlan startPlanForLocation(
    const kiriview::DisplayedImageLocation &location,
    kiriview::PredecodeMomentumMode momentumMode = kiriview::PredecodeMomentumMode::Neutral,
    bool powerSaverEnabled = false, int idealThreadCount = 4)
{
    return kiriview::predecodeWindowStartPlan(kiriview::PredecodeWindowPlanRequest {
        location,
        policyInputForLocation(location, momentumMode, powerSaverEnabled, idealThreadCount),
    });
}

}

class TestPredecodeWindowPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void regularImagePlansCandidateContextAndNeutralWindow();
    void powerSaverSuppressesCandidateLoading();
    void missingCandidateContextStillCarriesFallbackWindow();
    void directoryCollectionUsesDocumentParallelLimit();
    void predecodeWindowSkipsOpenedCollectionVideoCandidates();
    void archiveWindowPreservesOpenedCollectionScopeContext();
    void missingCurrentCandidateYieldsEmptyWindow();
    void candidateListingFailureUsesPlannedFallbackWindow();
};

void TestPredecodeWindowPlan::regularImagePlansCandidateContextAndNeutralWindow()
{
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(5));
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    QVERIFY(startPlan.shouldLoadCandidates());
    QVERIFY(startPlan.fallbackWindow.openedCollectionScope.isEmpty());
    QCOMPARE(startPlan.fallbackWindow.parallelLimit, std::size_t(1));
    QVERIFY(startPlan.fallbackWindow.urls.empty());
    QVERIFY(startPlan.candidateList.has_value());
    QCOMPARE(startPlan.candidateList->context.currentUrl(), indexedImageUrl(5));

    const kiriview::PredecodeWindowPlan windowPlan
        = kiriview::predecodeWindowPlanForCandidates(startPlan, imageDocumentPageCandidates(15));

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QCOMPARE(windowPlan.urls.size(), std::size_t(4));
    QCOMPARE(windowPlan.urls.at(0), indexedImageUrl(5));
    QCOMPARE(windowPlan.urls.at(1), indexedImageUrl(6));
    QCOMPARE(windowPlan.urls.at(2), indexedImageUrl(4));
    QCOMPARE(windowPlan.urls.at(3), indexedImageUrl(7));
}

void TestPredecodeWindowPlan::powerSaverSuppressesCandidateLoading()
{
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(5));
    const kiriview::PredecodeWindowStartPlan startPlan
        = startPlanForLocation(location, kiriview::PredecodeMomentumMode::Neutral, true);

    QVERIFY(!startPlan.shouldLoadCandidates());
    QVERIFY(!startPlan.candidateList.has_value());

    const kiriview::PredecodeWindowPlan windowPlan = startPlan.fallbackWindow;
    QCOMPARE(windowPlan.parallelLimit, std::size_t(0));
    QVERIFY(windowPlan.urls.empty());
}

void TestPredecodeWindowPlan::missingCandidateContextStillCarriesFallbackWindow()
{
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(QUrl());
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    QVERIFY(!startPlan.shouldLoadCandidates());
    QVERIFY(!startPlan.candidateList.has_value());

    const kiriview::PredecodeWindowPlan windowPlan = startPlan.fallbackWindow;
    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.openedCollectionScope.isEmpty());
    QVERIFY(windowPlan.urls.empty());
}

void TestPredecodeWindowPlan::directoryCollectionUsesDocumentParallelLimit()
{
    const kiriview::OpenedCollectionScopeLocation directoryCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), kiriview::OpenedCollectionScopeKind::Directory);
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            indexedImageUrl(5), directoryCollection);
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    QVERIFY(startPlan.shouldLoadCandidates());
    QCOMPARE(startPlan.fallbackWindow.openedCollectionScope, directoryCollection);

    const kiriview::PredecodeWindowPlan windowPlan
        = kiriview::predecodeWindowPlanForCandidates(startPlan, imageDocumentPageCandidates(15));

    QCOMPARE(windowPlan.parallelLimit, std::size_t(2));
    QVERIFY(windowPlan.urls.size() >= 2);
    QCOMPARE(windowPlan.urls.at(0), indexedImageUrl(5));
    QCOMPARE(windowPlan.urls.at(1), indexedImageUrl(6));
}

void TestPredecodeWindowPlan::predecodeWindowSkipsOpenedCollectionVideoCandidates()
{
    const kiriview::OpenedCollectionScopeLocation directoryCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), kiriview::OpenedCollectionScopeKind::Directory);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl videoUrl = localUrl(QStringLiteral("/images/02.mp4"));
    const QUrl nextImageUrl = indexedImageUrl(3);
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, directoryCollection);
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    const kiriview::PredecodeWindowPlan windowPlan
        = kiriview::predecodeWindowPlanForCandidates(startPlan,
            {
                imageDocumentPageCandidate(displayedUrl),
                videoCandidate(videoUrl),
                imageDocumentPageCandidate(nextImageUrl),
            });

    QCOMPARE(windowPlan.urls.size(), std::size_t(2));
    QCOMPARE(windowPlan.urls.at(0), displayedUrl);
    QCOMPARE(windowPlan.urls.at(1), nextImageUrl);
}

void TestPredecodeWindowPlan::archiveWindowPreservesOpenedCollectionScopeContext()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());

    const QUrl displayedUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("02.png"));
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *openedCollectionScope);
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    QVERIFY(startPlan.shouldLoadCandidates());
    QCOMPARE(startPlan.fallbackWindow.openedCollectionScope, *openedCollectionScope);

    const kiriview::PredecodeWindowPlan windowPlan
        = kiriview::predecodeWindowPlanForCandidates(startPlan,
            {
                imageDocumentPageCandidate(displayedUrl),
                imageDocumentPageCandidate(nextUrl),
            });

    QCOMPARE(windowPlan.openedCollectionScope, *openedCollectionScope);
    QCOMPARE(windowPlan.parallelLimit, std::size_t(2));
    QCOMPARE(windowPlan.urls.size(), std::size_t(2));
    QCOMPARE(windowPlan.urls.at(0), displayedUrl);
    QCOMPARE(windowPlan.urls.at(1), nextUrl);
}

void TestPredecodeWindowPlan::missingCurrentCandidateYieldsEmptyWindow()
{
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(5));
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    const kiriview::PredecodeWindowPlan windowPlan
        = kiriview::predecodeWindowPlanForCandidates(startPlan,
            {
                imageDocumentPageCandidate(indexedImageUrl(0)),
                imageDocumentPageCandidate(indexedImageUrl(1)),
                imageDocumentPageCandidate(indexedImageUrl(2)),
            });

    QCOMPARE(windowPlan.parallelLimit, std::size_t(1));
    QVERIFY(windowPlan.urls.empty());
}

void TestPredecodeWindowPlan::candidateListingFailureUsesPlannedFallbackWindow()
{
    const kiriview::DisplayedImageLocation location
        = kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(5));
    const kiriview::PredecodeWindowStartPlan startPlan = startPlanForLocation(location);

    QVERIFY(startPlan.shouldLoadCandidates());
    QCOMPARE(startPlan.fallbackWindow.parallelLimit, std::size_t(1));
    QVERIFY(startPlan.fallbackWindow.urls.empty());

    const kiriview::PredecodeWindowPlan fallbackWindow = kiriview::predecodeWindowPlanForCandidates(
        kiriview::PredecodeWindowStartPlan {
            startPlan.fallbackWindow,
            std::nullopt,
        },
        imageDocumentPageCandidates(15));

    QCOMPARE(fallbackWindow.parallelLimit, startPlan.fallbackWindow.parallelLimit);
    QVERIFY(fallbackWindow.urls.empty());
}

QTEST_GUILESS_MAIN(TestPredecodeWindowPlan)

#include "test_predecodewindowplan.moc"
