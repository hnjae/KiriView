// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecontaineropenplan.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <type_traits>

namespace {
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageCandidateListSource;
using KiriView::ImageContainerOpenError;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

template <typename ExpectedSource>
const ExpectedSource *typedSource(const ImageCandidateListSource &source)
{
    return source.visit([](const auto &typedSource) -> const ExpectedSource * {
        using Source = std::decay_t<decltype(typedSource)>;
        if constexpr (std::is_same_v<Source, ExpectedSource>) {
            return &typedSource;
        } else {
            return nullptr;
        }
    });
}
}

class TestImageContainerOpenPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directoryContainerPlansDirectoryListing();
    void comicBookArchiveContainerPlansArchiveListing();
    void invalidArchiveContainerReportsTypedError();
    void firstCandidateSelectionReportsImageOrEmptyContainer();
};

void TestImageContainerOpenPlan::directoryContainerPlansDirectoryListing()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    const KiriView::ImageContainerOpenPlan plan
        = KiriView::imageContainerOpenPlanForCandidate(KiriView::ContainerNavigationCandidate {
            containerUrl, QStringLiteral("a"), ContainerNavigationCandidateType::Directory });

    QVERIFY(plan.shouldLoadCandidates());
    const auto *directory = typedSource<ImageCandidateListSource::Directory>(*plan.source);
    QVERIFY(directory != nullptr);
    QCOMPARE(directory->directoryUrl, containerUrl);
}

void TestImageContainerOpenPlan::comicBookArchiveContainerPlansArchiveListing()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(containerUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageContainerOpenPlan plan = KiriView::imageContainerOpenPlanForCandidate(
        KiriView::ContainerNavigationCandidate { containerUrl, QStringLiteral("book.cbz"),
            ContainerNavigationCandidateType::ComicBookArchive });

    QVERIFY(plan.shouldLoadCandidates());
    const auto *archive = typedSource<ImageCandidateListSource::ArchiveDocument>(*plan.source);
    QVERIFY(archive != nullptr);
    QCOMPARE(archive->archiveDocument.rootUrl(), archiveDocument->rootUrl());
    QVERIFY(archive->archiveDocument.isComicBook());
}

void TestImageContainerOpenPlan::invalidArchiveContainerReportsTypedError()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/not-an-archive.png"));
    const KiriView::ImageContainerOpenPlan plan = KiriView::imageContainerOpenPlanForCandidate(
        KiriView::ContainerNavigationCandidate { containerUrl, QStringLiteral("not-an-archive.png"),
            ContainerNavigationCandidateType::ComicBookArchive });

    QVERIFY(!plan.shouldLoadCandidates());
    QCOMPARE(plan.error, ImageContainerOpenError::InvalidComicBookArchive);
}

void TestImageContainerOpenPlan::firstCandidateSelectionReportsImageOrEmptyContainer()
{
    const QUrl firstImageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    const KiriView::ImageContainerOpenResult opened
        = KiriView::imageContainerOpenResultForCandidates({ imageCandidate(firstImageUrl),
            imageCandidate(localUrl(QStringLiteral("/books/a/02.png"))) });

    QVERIFY(opened.openedImage());
    QCOMPARE(*opened.imageUrl, firstImageUrl);

    const KiriView::ImageContainerOpenResult empty
        = KiriView::imageContainerOpenResultForCandidates({});
    QVERIFY(!empty.openedImage());
    QCOMPARE(empty.error, ImageContainerOpenError::EmptyContainer);
}

QTEST_GUILESS_MAIN(TestImageContainerOpenPlan)

#include "test_imagecontaineropenplan.moc"
