// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecandidateitems.h"

#include <KFileItem>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <sys/stat.h>
#include <vector>

class TestImageCandidateItems : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageCandidatesOnlyIncludeSupportedMediaFiles();
    void containerCandidatesOnlyIncludeComicBookArchives();
};

void TestImageCandidateItems::imageCandidatesOnlyIncludeSupportedMediaFiles()
{
    KFileItemList items;
    items.append(KFileItem(QUrl::fromLocalFile(QStringLiteral("/images/a/")), QString(), S_IFDIR));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/images/02.txt")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/images/02.png")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/images/03.mp4")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/images/01.jpg")), QString(), S_IFREG));

    const std::vector<KiriView::ImageNavigationCandidate> candidates
        = KiriView::imageNavigationCandidates(items);

    QCOMPARE(candidates.size(), std::size_t(3));
    QCOMPARE(candidates.front().url, QUrl::fromLocalFile(QStringLiteral("/images/01.jpg")));
    QCOMPARE(candidates.front().name, QStringLiteral("01.jpg"));
    QCOMPARE(candidates.front().kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(candidates.at(1).url, QUrl::fromLocalFile(QStringLiteral("/images/02.png")));
    QCOMPARE(candidates.at(1).name, QStringLiteral("02.png"));
    QCOMPARE(candidates.at(1).kind, KiriView::ImageNavigationCandidateKind::Image);
    QCOMPARE(candidates.back().url, QUrl::fromLocalFile(QStringLiteral("/images/03.mp4")));
    QCOMPARE(candidates.back().name, QStringLiteral("03.mp4"));
    QCOMPARE(candidates.back().kind, KiriView::ImageNavigationCandidateKind::Video);
}

void TestImageCandidateItems::containerCandidatesOnlyIncludeComicBookArchives()
{
    KFileItemList items;
    items.append(KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/a/")), QString(), S_IFDIR));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/a.cbz")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/b.cbr")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/book.zip")), QString(), S_IFREG));
    items.append(
        KFileItem(QUrl::fromLocalFile(QStringLiteral("/books/book.rar")), QString(), S_IFREG));

    const std::vector<KiriView::ContainerNavigationCandidate> candidates
        = KiriView::containerNavigationCandidates(items);
    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(candidates.front().url, QUrl::fromLocalFile(QStringLiteral("/books/a.cbz")));
    QCOMPARE(candidates.front().type, KiriView::ContainerNavigationCandidateType::ComicBookArchive);
    QCOMPARE(candidates.back().url, QUrl::fromLocalFile(QStringLiteral("/books/b.cbr")));
    QCOMPARE(candidates.back().type, KiriView::ContainerNavigationCandidateType::ComicBookArchive);
}

QTEST_GUILESS_MAIN(TestImageCandidateItems)

#include "test_imagecandidateitems.moc"
