// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimagenavigation.h"

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

QUrl imageUrl(int index)
{
    return QUrl(QStringLiteral("file:///images/%1.png").arg(index, 2, 10, QLatin1Char('0')));
}

std::vector<ImageNavigationCandidate> imageCandidates(int count)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(ImageNavigationCandidate {
            imageUrl(index), QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0')) });
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

class TestKiriImageNavigation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adjacentImageNavigationDoesNotWrap();
    void adjacentContainerNavigationUsesTheSameRules();
    void pageNavigationInsertsFallbackCurrentUrl();
    void predecodeWindowPrioritizesCurrentAndForwardPages();
    void comicBookArchiveUrlsResolveToTheirContainer();
    void regularImageContainerUrlsResolveToParentDirectory();
};

void TestKiriImageNavigation::adjacentImageNavigationDoesNotWrap()
{
    const std::vector<ImageNavigationCandidate> candidates = imageCandidates(3);

    const std::optional<QUrl> previous = KiriView::adjacentImageNavigationUrl(
        candidates, imageUrl(1), NavigationDirection::Previous);
    QVERIFY(previous.has_value());
    QCOMPARE(*previous, imageUrl(0));

    const std::optional<QUrl> next
        = KiriView::adjacentImageNavigationUrl(candidates, imageUrl(1), NavigationDirection::Next);
    QVERIFY(next.has_value());
    QCOMPARE(*next, imageUrl(2));

    QVERIFY(!KiriView::adjacentImageNavigationUrl(
        candidates, imageUrl(0), NavigationDirection::Previous)
            .has_value());
    QVERIFY(
        !KiriView::adjacentImageNavigationUrl(candidates, imageUrl(2), NavigationDirection::Next)
            .has_value());
    QVERIFY(
        !KiriView::adjacentImageNavigationUrl(candidates, imageUrl(9), NavigationDirection::Next)
            .has_value());
}

void TestKiriImageNavigation::adjacentContainerNavigationUsesTheSameRules()
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

void TestKiriImageNavigation::pageNavigationInsertsFallbackCurrentUrl()
{
    PageNavigationState state
        = KiriView::pageNavigationStateForUrls({ imageUrl(0), imageUrl(1) }, imageUrl(1));
    QCOMPARE(state.currentIndex, 1);
    compareUrls(state.urls, { imageUrl(0), imageUrl(1) });

    state = KiriView::pageNavigationStateForUrls({ imageUrl(0), imageUrl(1) }, imageUrl(9));
    QCOMPARE(state.currentIndex, 0);
    compareUrls(state.urls, { imageUrl(9), imageUrl(0), imageUrl(1) });
}

void TestKiriImageNavigation::predecodeWindowPrioritizesCurrentAndForwardPages()
{
    const std::vector<QUrl> urls
        = KiriView::predecodeWindowImageUrls(imageCandidates(15), imageUrl(5));

    compareUrls(urls,
        { imageUrl(5), imageUrl(6), imageUrl(4), imageUrl(7), imageUrl(3), imageUrl(8), imageUrl(9),
            imageUrl(10), imageUrl(11), imageUrl(12), imageUrl(13), imageUrl(14) });
}

void TestKiriImageNavigation::comicBookArchiveUrlsResolveToTheirContainer()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    QCOMPARE(archiveRootUrl->scheme(), QStringLiteral("zip"));
    QCOMPARE(archiveRootUrl->path(), QStringLiteral("/books/book.cbz/"));

    QUrl pageUrl = *archiveRootUrl;
    pageUrl.setPath(archiveRootUrl->path() + QStringLiteral("chapter/page001.png"));

    QVERIFY(KiriView::isUrlInsideArchiveRoot(pageUrl, *archiveRootUrl));
    QCOMPARE(KiriView::containingComicBookArchiveRootUrl(pageUrl), archiveRootUrl);
    QCOMPARE(KiriView::windowTitleFileNameForDisplayedUrl(pageUrl, *archiveRootUrl),
        QStringLiteral("book.cbz"));
    QCOMPARE(KiriView::containerNavigationUrlForImage(pageUrl, *archiveRootUrl), archiveUrl);
}

void TestKiriImageNavigation::regularImageContainerUrlsResolveToParentDirectory()
{
    const QUrl fileUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const QUrl parentUrl = QUrl::fromLocalFile(QStringLiteral("/images/"));

    QVERIFY(KiriView::sameContainerNavigationUrl(
        KiriView::containerNavigationUrlForImage(fileUrl, QUrl()), parentUrl));
    QVERIFY(
        KiriView::sameContainerNavigationUrl(KiriView::parentUrlForContainerNavigation(parentUrl),
            QUrl::fromLocalFile(QStringLiteral("/"))));
}

QTEST_GUILESS_MAIN(TestKiriImageNavigation)

#include "test_kiriimagenavigation.moc"
