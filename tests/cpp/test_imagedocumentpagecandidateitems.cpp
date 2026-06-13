// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidateitems.h"

#include <KFileItem>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <sys/stat.h>
#include <vector>

class TestImageDocumentPageCandidateItems : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageDocumentPageCandidatesOnlyIncludeSupportedMediaFiles();
    void containerCandidatesOnlyIncludeComicBookArchives();
};

void TestImageDocumentPageCandidateItems::
    imageDocumentPageCandidatesOnlyIncludeSupportedMediaFiles()
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

    const std::vector<kiriview::ImageDocumentPageCandidate> candidates
        = kiriview::imageDocumentPageNavigationCandidates(items);

    QCOMPARE(candidates.size(), std::size_t(3));
    QCOMPARE(candidates.front().url, QUrl::fromLocalFile(QStringLiteral("/images/01.jpg")));
    QCOMPARE(candidates.front().name, QStringLiteral("01.jpg"));
    QCOMPARE(candidates.front().kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(candidates.at(1).url, QUrl::fromLocalFile(QStringLiteral("/images/02.png")));
    QCOMPARE(candidates.at(1).name, QStringLiteral("02.png"));
    QCOMPARE(candidates.at(1).kind, kiriview::ImageDocumentPageKind::Image);
    QCOMPARE(candidates.back().url, QUrl::fromLocalFile(QStringLiteral("/images/03.mp4")));
    QCOMPARE(candidates.back().name, QStringLiteral("03.mp4"));
    QCOMPARE(candidates.back().kind, kiriview::ImageDocumentPageKind::Video);

    const std::vector<kiriview::DirectMediaNavigationCandidate> directMediaNavigationCandidates
        = kiriview::directMediaNavigationCandidates(items);

    QCOMPARE(directMediaNavigationCandidates.size(), candidates.size());
    QCOMPARE(directMediaNavigationCandidates.front().url, candidates.front().url);
    QCOMPARE(directMediaNavigationCandidates.front().name, candidates.front().name);
    QCOMPARE(directMediaNavigationCandidates.at(1).url, candidates.at(1).url);
    QCOMPARE(directMediaNavigationCandidates.at(1).name, candidates.at(1).name);
    QCOMPARE(directMediaNavigationCandidates.back().url, candidates.back().url);
    QCOMPARE(directMediaNavigationCandidates.back().name, candidates.back().name);
}

void TestImageDocumentPageCandidateItems::containerCandidatesOnlyIncludeComicBookArchives()
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

    const std::vector<kiriview::ContainerNavigationCandidate> candidates
        = kiriview::containerNavigationCandidates(items);
    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(candidates.front().url, QUrl::fromLocalFile(QStringLiteral("/books/a.cbz")));
    QCOMPARE(candidates.front().type, kiriview::ContainerNavigationCandidateType::ComicBookArchive);
    QCOMPARE(candidates.back().url, QUrl::fromLocalFile(QStringLiteral("/books/b.cbr")));
    QCOMPARE(candidates.back().type, kiriview::ContainerNavigationCandidateType::ComicBookArchive);
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateItems)

#include "test_imagedocumentpagecandidateitems.moc"
