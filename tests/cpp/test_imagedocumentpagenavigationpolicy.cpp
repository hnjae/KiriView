// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagenavigationpolicy.h"

#include "candidate_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using kiriview::ContainerNavigationCandidate;
using kiriview::ContainerNavigationCandidateType;
using kiriview::ImageDocumentPageCandidate;
using kiriview::NavigationDirection;
using kiriview::PageNavigationState;
using kiriview::TestSupport::indexedImageFileName;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::localUrl;

std::vector<ImageDocumentPageCandidate> imageDocumentPageCandidates(int count)
{
    std::vector<ImageDocumentPageCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(
            ImageDocumentPageCandidate { indexedImageUrl(index), indexedImageFileName(index) });
    }
    return candidates;
}

void compareUrls(const std::vector<QUrl>& actual, const std::vector<QUrl>& expected)
{
    QCOMPARE(static_cast<int>(actual.size()), static_cast<int>(expected.size()));
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageDocumentPageNavigationPolicy : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageDocumentPageNavigationCandidateProjectionsUseNormalizedUrlIdentity();
    void adjacentImageDocumentPageNavigationDoesNotWrap();
    void adjacentContainerNavigationUsesTheSameRules();
    void pageNavigationStateProjectionsExposeCanonicalPublicValues();
    void pageNavigationTargetSkipsInvalidCurrentAndOutOfRangePages();
    void pageNavigationAdjacentTargetUsesKnownCurrentIndex();
    void pageNavigationPreviewReusesKnownList();
    void pageNavigationPreviewKeepsKnownListWhenCurrentTemporarilyMissing();
    void pageNavigationInsertsFallbackCurrentUrl();
    void pageNavigationUpdateKeepsCandidateListWhenCurrentMissing();
};

void TestImageDocumentPageNavigationPolicy::
    imageDocumentPageNavigationCandidateProjectionsUseNormalizedUrlIdentity()
{
    const std::vector<ImageDocumentPageCandidate> candidates = imageDocumentPageCandidates(3);
    const std::vector<QUrl> urls = kiriview::imageDocumentPageCandidateUrls(candidates);
    const std::vector<kiriview::ImageDocumentPageTarget> targets
        = kiriview::imageDocumentPageCandidateTargets(candidates);

    compareUrls(urls, { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) });
    QCOMPARE(targets.at(1).name, indexedImageFileName(1));
    const std::optional<std::size_t> secondIndex = kiriview::imageDocumentPageCandidateIndex(
        candidates, localUrl(QStringLiteral("/images/subdirectory/../01.png")));
    QVERIFY(secondIndex.has_value());
    QCOMPARE(*secondIndex, std::size_t(1));
    QVERIFY(kiriview::imageDocumentPageCandidatesContainUrl(candidates, indexedImageUrl(2)));
    QVERIFY(!kiriview::imageDocumentPageCandidatesContainUrl(candidates, indexedImageUrl(9)));
}

void TestImageDocumentPageNavigationPolicy::adjacentImageDocumentPageNavigationDoesNotWrap()
{
    const std::vector<ImageDocumentPageCandidate> candidates = imageDocumentPageCandidates(3);

    const std::optional<QUrl> previous = kiriview::adjacentImageDocumentPageUrl(
        candidates, indexedImageUrl(1), NavigationDirection::Previous);
    QVERIFY(previous.has_value());
    QCOMPARE(*previous, indexedImageUrl(0));

    const std::optional<QUrl> next = kiriview::adjacentImageDocumentPageUrl(
        candidates, indexedImageUrl(1), NavigationDirection::Next);
    QVERIFY(next.has_value());
    QCOMPARE(*next, indexedImageUrl(2));

    QVERIFY(!kiriview::adjacentImageDocumentPageUrl(
        candidates, indexedImageUrl(0), NavigationDirection::Previous)
            .has_value());
    QVERIFY(!kiriview::adjacentImageDocumentPageUrl(
        candidates, indexedImageUrl(2), NavigationDirection::Next)
            .has_value());
    QVERIFY(!kiriview::adjacentImageDocumentPageUrl(
        candidates, indexedImageUrl(9), NavigationDirection::Next)
            .has_value());
}

void TestImageDocumentPageNavigationPolicy::adjacentContainerNavigationUsesTheSameRules()
{
    const QUrl first = QUrl::fromLocalFile(QStringLiteral("/books/a/"));
    const QUrl second = QUrl::fromLocalFile(QStringLiteral("/books/b.cbz"));
    const QUrl third = QUrl::fromLocalFile(QStringLiteral("/books/c/"));
    const std::vector<ContainerNavigationCandidate> candidates = {
        { first, QStringLiteral("a"), ContainerNavigationCandidateType::Directory },
        { second, QStringLiteral("b.cbz"), ContainerNavigationCandidateType::ComicBookArchive },
        { third, QStringLiteral("c"), ContainerNavigationCandidateType::Directory },
    };

    const std::optional<ContainerNavigationCandidate> previous
        = kiriview::adjacentContainerNavigationCandidate(
            candidates, second, NavigationDirection::Previous);
    QVERIFY(previous.has_value());
    QCOMPARE(previous->url, first);
    QCOMPARE(previous->type, ContainerNavigationCandidateType::Directory);

    const std::optional<ContainerNavigationCandidate> next
        = kiriview::adjacentContainerNavigationCandidate(
            candidates, second, NavigationDirection::Next);
    QVERIFY(next.has_value());
    QCOMPARE(next->url, third);

    QVERIFY(!kiriview::adjacentContainerNavigationCandidate(
        candidates, first, NavigationDirection::Previous)
            .has_value());
    QVERIFY(!kiriview::adjacentContainerNavigationCandidate(
        candidates, third, NavigationDirection::Next)
            .has_value());
}

void TestImageDocumentPageNavigationPolicy::
    pageNavigationStateProjectionsExposeCanonicalPublicValues()
{
    const PageNavigationState state {
        { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) },
        1,
    };

    QCOMPARE(kiriview::pageNavigationCurrentPageNumber(state), 2);
    QCOMPARE(kiriview::pageNavigationPageCount(state), 3);
    QVERIFY(kiriview::pageNavigationHasKnownSelection(state));
    QVERIFY(kiriview::pageNavigationUrlAtPage(state, 2).has_value());
    QCOMPARE(*kiriview::pageNavigationUrlAtPage(state, 2), indexedImageUrl(1));
    QVERIFY(!kiriview::pageNavigationUrlAtPage(state, 0).has_value());
    QVERIFY(!kiriview::pageNavigationUrlAtPage(state, 4).has_value());
    QVERIFY(kiriview::samePageNavigationState(state,
        PageNavigationState { { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) }, 1 }));
    QVERIFY(!kiriview::samePageNavigationState(state,
        PageNavigationState { { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) }, 2 }));

    const PageNavigationState unknownSelection {
        { indexedImageUrl(0) },
        -1,
    };
    QCOMPARE(kiriview::pageNavigationCurrentPageNumber(unknownSelection), 0);
    QVERIFY(!kiriview::pageNavigationHasKnownSelection(unknownSelection));
}

void TestImageDocumentPageNavigationPolicy::
    pageNavigationTargetSkipsInvalidCurrentAndOutOfRangePages()
{
    PageNavigationState state { { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) }, 1 };

    std::optional<std::size_t> target = kiriview::pageNavigationTargetIndex(state, 1);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(0));

    target = kiriview::pageNavigationTargetIndex(state, 2);
    QVERIFY(!target.has_value());

    target = kiriview::pageNavigationTargetIndex(state, 0);
    QVERIFY(!target.has_value());

    target = kiriview::pageNavigationTargetIndex(state, 4);
    QVERIFY(!target.has_value());
}

void TestImageDocumentPageNavigationPolicy::pageNavigationAdjacentTargetUsesKnownCurrentIndex()
{
    PageNavigationState state { { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) }, 1 };

    std::optional<std::size_t> target
        = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Previous);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(0));

    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Next);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(2));

    state.currentIndex = 0;
    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Previous);
    QVERIFY(!target.has_value());

    state.currentIndex = 2;
    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Next);
    QVERIFY(!target.has_value());

    state.currentIndex = -1;
    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Next);
    QVERIFY(!target.has_value());

    state.currentIndex = 9;
    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Next);
    QVERIFY(!target.has_value());

    state.targets.clear();
    state.currentIndex = 0;
    target = kiriview::pageNavigationAdjacentTargetIndex(state, NavigationDirection::Next);
    QVERIFY(!target.has_value());
}

void TestImageDocumentPageNavigationPolicy::pageNavigationPreviewReusesKnownList()
{
    const PageNavigationState knownState {
        { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) },
        0,
    };

    PageNavigationState state
        = kiriview::pageNavigationStateForCurrentUrl(knownState, indexedImageUrl(2));
    QCOMPARE(state.currentIndex, 2);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        kiriview::imageDocumentPageTargetUrls(knownState.targets));

    state = kiriview::pageNavigationStateForCurrentUrl(knownState, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, -1);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        kiriview::imageDocumentPageTargetUrls(knownState.targets));

    state = kiriview::pageNavigationStateForCurrentUrl(knownState, QUrl());
    QCOMPARE(state.currentIndex, -1);
    QVERIFY(state.targets.empty());

    state = kiriview::pageNavigationStateForCurrentUrl(PageNavigationState {}, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, -1);
    QVERIFY(state.targets.empty());
}

void TestImageDocumentPageNavigationPolicy::
    pageNavigationPreviewKeepsKnownListWhenCurrentTemporarilyMissing()
{
    const PageNavigationState knownState {
        { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) },
        0,
    };

    const PageNavigationState state
        = kiriview::pageNavigationStateForCurrentUrl(knownState, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, -1);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        kiriview::imageDocumentPageTargetUrls(knownState.targets));

    PageNavigationState emptyState {};
    const PageNavigationState unknownState
        = kiriview::pageNavigationStateForCurrentUrl(emptyState, indexedImageUrl(9));
    QCOMPARE(unknownState.currentIndex, -1);
    QVERIFY(unknownState.targets.empty());
}

void TestImageDocumentPageNavigationPolicy::pageNavigationInsertsFallbackCurrentUrl()
{
    PageNavigationState state = kiriview::pageNavigationStateForUrls(
        { indexedImageUrl(0), indexedImageUrl(1) }, indexedImageUrl(1));
    QCOMPARE(state.currentIndex, 1);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        { indexedImageUrl(0), indexedImageUrl(1) });

    state = kiriview::pageNavigationStateForUrls(
        { indexedImageUrl(0), indexedImageUrl(1) }, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, -1);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        { indexedImageUrl(0), indexedImageUrl(1) });
}

void TestImageDocumentPageNavigationPolicy::
    pageNavigationUpdateKeepsCandidateListWhenCurrentMissing()
{
    PageNavigationState state = kiriview::pageNavigationStateForUrls(
        { indexedImageUrl(0), indexedImageUrl(1) }, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, -1);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets),
        { indexedImageUrl(0), indexedImageUrl(1) });

    state = kiriview::pageNavigationStateForUrls({}, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, 0);
    compareUrls(kiriview::imageDocumentPageTargetUrls(state.targets), { indexedImageUrl(9) });
}

QTEST_GUILESS_MAIN(TestImageDocumentPageNavigationPolicy)

#include "test_imagedocumentpagenavigationpolicy.moc"
