// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationmodel.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidate;
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::PageNavigationState;
using KiriView::TestSupport::indexedImageFileName;
using KiriView::TestSupport::indexedImageUrl;

std::vector<ImageNavigationCandidate> imageCandidates(int count)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(
            ImageNavigationCandidate { indexedImageUrl(index), indexedImageFileName(index) });
    }
    return candidates;
}

void compareUrls(const std::vector<QUrl> &actual, const std::vector<QUrl> &expected)
{
    QCOMPARE(static_cast<int>(actual.size()), static_cast<int>(expected.size()));
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}
}

class TestImageNavigationModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adjacentImageNavigationDoesNotWrap();
    void adjacentContainerNavigationUsesTheSameRules();
    void pageNavigationTargetSkipsInvalidCurrentAndOutOfRangePages();
    void pageNavigationInsertsFallbackCurrentUrl();
    void predecodeWindowKeepsTwoPreviousAndFourNextPages();
};

void TestImageNavigationModel::adjacentImageNavigationDoesNotWrap()
{
    const std::vector<ImageNavigationCandidate> candidates = imageCandidates(3);

    const std::optional<QUrl> previous = KiriView::adjacentImageNavigationUrl(
        candidates, indexedImageUrl(1), NavigationDirection::Previous);
    QVERIFY(previous.has_value());
    QCOMPARE(*previous, indexedImageUrl(0));

    const std::optional<QUrl> next = KiriView::adjacentImageNavigationUrl(
        candidates, indexedImageUrl(1), NavigationDirection::Next);
    QVERIFY(next.has_value());
    QCOMPARE(*next, indexedImageUrl(2));

    QVERIFY(!KiriView::adjacentImageNavigationUrl(
        candidates, indexedImageUrl(0), NavigationDirection::Previous)
            .has_value());
    QVERIFY(!KiriView::adjacentImageNavigationUrl(
        candidates, indexedImageUrl(2), NavigationDirection::Next)
            .has_value());
    QVERIFY(!KiriView::adjacentImageNavigationUrl(
        candidates, indexedImageUrl(9), NavigationDirection::Next)
            .has_value());
}

void TestImageNavigationModel::adjacentContainerNavigationUsesTheSameRules()
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
        = KiriView::adjacentContainerNavigationCandidate(
            candidates, second, NavigationDirection::Previous);
    QVERIFY(previous.has_value());
    QCOMPARE(previous->url, first);
    QCOMPARE(previous->type, ContainerNavigationCandidateType::Directory);

    const std::optional<ContainerNavigationCandidate> next
        = KiriView::adjacentContainerNavigationCandidate(
            candidates, second, NavigationDirection::Next);
    QVERIFY(next.has_value());
    QCOMPARE(next->url, third);

    QVERIFY(!KiriView::adjacentContainerNavigationCandidate(
        candidates, first, NavigationDirection::Previous)
            .has_value());
    QVERIFY(!KiriView::adjacentContainerNavigationCandidate(
        candidates, third, NavigationDirection::Next)
            .has_value());
}

void TestImageNavigationModel::pageNavigationTargetSkipsInvalidCurrentAndOutOfRangePages()
{
    PageNavigationState state { { indexedImageUrl(0), indexedImageUrl(1), indexedImageUrl(2) }, 1 };

    std::optional<std::size_t> target = KiriView::pageNavigationTargetIndex(state, 1);
    QVERIFY(target.has_value());
    QCOMPARE(*target, std::size_t(0));

    target = KiriView::pageNavigationTargetIndex(state, 2);
    QVERIFY(!target.has_value());

    target = KiriView::pageNavigationTargetIndex(state, 0);
    QVERIFY(!target.has_value());

    target = KiriView::pageNavigationTargetIndex(state, 4);
    QVERIFY(!target.has_value());
}

void TestImageNavigationModel::pageNavigationInsertsFallbackCurrentUrl()
{
    PageNavigationState state = KiriView::pageNavigationStateForUrls(
        { indexedImageUrl(0), indexedImageUrl(1) }, indexedImageUrl(1));
    QCOMPARE(state.currentIndex, 1);
    compareUrls(state.urls, { indexedImageUrl(0), indexedImageUrl(1) });

    state = KiriView::pageNavigationStateForUrls(
        { indexedImageUrl(0), indexedImageUrl(1) }, indexedImageUrl(9));
    QCOMPARE(state.currentIndex, 0);
    compareUrls(state.urls, { indexedImageUrl(9), indexedImageUrl(0), indexedImageUrl(1) });
}

void TestImageNavigationModel::predecodeWindowKeepsTwoPreviousAndFourNextPages()
{
    const std::vector<QUrl> urls
        = KiriView::predecodeWindowImageUrls(imageCandidates(15), indexedImageUrl(5));

    compareUrls(urls,
        { indexedImageUrl(5), indexedImageUrl(6), indexedImageUrl(4), indexedImageUrl(7),
            indexedImageUrl(3), indexedImageUrl(8), indexedImageUrl(9) });
}

QTEST_GUILESS_MAIN(TestImageNavigationModel)

#include "test_imagenavigationmodel.moc"
