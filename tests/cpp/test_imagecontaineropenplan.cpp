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
using kiriview::ContainerNavigationCandidateType;
using kiriview::ImageContainerOpenError;
using kiriview::ImageDocumentPageCandidateListSource;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;

template <typename ExpectedSource>
const ExpectedSource* typedSource(const ImageDocumentPageCandidateListSource& source)
{
    return source.visit([](const auto& typedSource) -> const ExpectedSource* {
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
    const kiriview::ImageContainerOpenPlan plan
        = kiriview::imageContainerOpenPlanForCandidate(kiriview::ContainerNavigationCandidate {
            containerUrl, QStringLiteral("a"), ContainerNavigationCandidateType::Directory });

    QVERIFY(plan.shouldLoadCandidates());
    const auto* directory
        = typedSource<ImageDocumentPageCandidateListSource::Directory>(*plan.source);
    QVERIFY(directory != nullptr);
    QCOMPARE(directory->directoryUrl, containerUrl);
}

void TestImageContainerOpenPlan::comicBookArchiveContainerPlansArchiveListing()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(containerUrl);
    QVERIFY(openedCollectionScope.has_value());

    const kiriview::ImageContainerOpenPlan plan = kiriview::imageContainerOpenPlanForCandidate(
        kiriview::ContainerNavigationCandidate { containerUrl, QStringLiteral("book.cbz"),
            ContainerNavigationCandidateType::ComicBookArchive });

    QVERIFY(plan.shouldLoadCandidates());
    const auto* archive
        = typedSource<ImageDocumentPageCandidateListSource::OpenedCollectionScope>(*plan.source);
    QVERIFY(archive != nullptr);
    QCOMPARE(archive->openedCollectionScope.rootUrl(), openedCollectionScope->rootUrl());
    QVERIFY(archive->openedCollectionScope.isComicBook());
}

void TestImageContainerOpenPlan::invalidArchiveContainerReportsTypedError()
{
    const QUrl containerUrl = localUrl(QStringLiteral("/books/not-an-archive.png"));
    const kiriview::ImageContainerOpenPlan plan = kiriview::imageContainerOpenPlanForCandidate(
        kiriview::ContainerNavigationCandidate { containerUrl, QStringLiteral("not-an-archive.png"),
            ContainerNavigationCandidateType::ComicBookArchive });

    QVERIFY(!plan.shouldLoadCandidates());
    QCOMPARE(plan.error, ImageContainerOpenError::InvalidComicBookArchive);
}

void TestImageContainerOpenPlan::firstCandidateSelectionReportsImageOrEmptyContainer()
{
    const QUrl firstImageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    const kiriview::ImageContainerOpenResult opened
        = kiriview::imageContainerOpenResultForCandidates(
            { kiriview::ImageDocumentPageCandidate {
                  firstImageUrl, QStringLiteral("01.bin"), kiriview::ImageDocumentPageKind::Video },
                imageDocumentPageCandidate(localUrl(QStringLiteral("/books/a/02.png"))) });

    QVERIFY(opened.openedImage());
    QCOMPARE(opened.target->url, firstImageUrl);
    QCOMPARE(opened.target->kind, kiriview::ImageDocumentPageKind::Video);

    const kiriview::ImageContainerOpenResult empty
        = kiriview::imageContainerOpenResultForCandidates({});
    QVERIFY(!empty.openedImage());
    QCOMPARE(empty.error, ImageContainerOpenError::EmptyContainer);
}

QTEST_GUILESS_MAIN(TestImageContainerOpenPlan)

#include "test_imagecontaineropenplan.moc"
