// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidatelistsource.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <type_traits>

namespace {
using KiriView::ImageDocumentPageCandidateListContext;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

using OpenedCollectionCandidateContext
    = ImageDocumentPageCandidateListContext::OpenedCollectionScopeContext;
using DirectoryCandidateContext = ImageDocumentPageCandidateListContext::DirectoryContext;

template <typename ExpectedContext>
const ExpectedContext *typedCandidateContext(const ImageDocumentPageCandidateListContext &context)
{
    return context.visit([](const auto &typedContext) -> const ExpectedContext * {
        using Context = std::decay_t<decltype(typedContext)>;
        if constexpr (std::is_same_v<Context, ExpectedContext>) {
            return &typedContext;
        } else {
            return nullptr;
        }
    });
}
}

class TestImageDocumentPageCandidateListSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageContextsSelectDirectoryOrArchiveListing();
    void candidateListIdentityComparesNormalizedSourcesAndCurrentUrls();
};

void TestImageDocumentPageCandidateListSource::
    displayedImageContextsSelectDirectoryOrArchiveListing()
{
    const QUrl fileUrl = localUrl(QStringLiteral("/images/02.png"));
    const std::optional<KiriView::ImageDocumentPageCandidateListContext> directoryContext
        = KiriView::imageDocumentPageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromUrl(fileUrl));
    QVERIFY(directoryContext.has_value());
    QCOMPARE(directoryContext->currentUrl(), fileUrl);
    const DirectoryCandidateContext *directory
        = typedCandidateContext<DirectoryCandidateContext>(*directoryContext);
    QVERIFY(directory != nullptr);
    QCOMPARE(directory->directoryUrl, localUrl(QStringLiteral("/images/")));
    QVERIFY(directoryContext->openedCollectionScope().isEmpty());

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl pageUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageDocumentPageCandidateListContext> archiveContext
        = KiriView::imageDocumentPageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                pageUrl, *openedCollectionScope));
    QVERIFY(archiveContext.has_value());
    QCOMPARE(archiveContext->currentUrl(), pageUrl);
    const OpenedCollectionCandidateContext *archive
        = typedCandidateContext<OpenedCollectionCandidateContext>(*archiveContext);
    QVERIFY(archive != nullptr);
    QCOMPARE(archive->openedCollectionScope.rootUrl(), openedCollectionScope->rootUrl());
    QCOMPARE(archiveContext->openedCollectionScope().rootUrl(), openedCollectionScope->rootUrl());

    const QUrl directArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> directArchiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(directArchiveUrl);
    QVERIFY(directArchiveCollection.has_value());
    const QUrl directArchivePageUrl
        = archivePageUrl(directArchiveCollection->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageDocumentPageCandidateListContext> directArchiveContext
        = KiriView::imageDocumentPageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                directArchivePageUrl, *directArchiveCollection));
    QVERIFY(directArchiveContext.has_value());
    QCOMPARE(directArchiveContext->currentUrl(), directArchivePageUrl);
    const OpenedCollectionCandidateContext *directArchive
        = typedCandidateContext<OpenedCollectionCandidateContext>(*directArchiveContext);
    QVERIFY(directArchive != nullptr);
    QCOMPARE(directArchive->openedCollectionScope.rootUrl(), directArchiveCollection->rootUrl());
    QCOMPARE(directArchiveContext->openedCollectionScope().rootUrl(),
        directArchiveCollection->rootUrl());

    const QUrl explicitArchiveImageUrl(QStringLiteral("zip:///books/book.cbz/chapter/02.png"));
    const std::optional<KiriView::ImageDocumentPageCandidateListContext> explicitArchiveContext
        = KiriView::imageDocumentPageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromUrl(explicitArchiveImageUrl));
    QVERIFY(explicitArchiveContext.has_value());
    QCOMPARE(explicitArchiveContext->currentUrl(), explicitArchiveImageUrl);
    const DirectoryCandidateContext *explicitArchiveDirectory
        = typedCandidateContext<DirectoryCandidateContext>(*explicitArchiveContext);
    QVERIFY(explicitArchiveDirectory != nullptr);
    QCOMPARE(explicitArchiveDirectory->directoryUrl,
        QUrl(QStringLiteral("zip:///books/book.cbz/chapter/")));
}

void TestImageDocumentPageCandidateListSource::
    candidateListIdentityComparesNormalizedSourcesAndCurrentUrls()
{
    const ImageDocumentPageCandidateListContext directoryContext
        = ImageDocumentPageCandidateListContext::forDirectory(
            localUrl(QStringLiteral("/images/01.png")), localUrl(QStringLiteral("/images/")));
    const ImageDocumentPageCandidateListContext normalizedDirectoryContext
        = ImageDocumentPageCandidateListContext::forDirectory(
            localUrl(QStringLiteral("/images/./01.png")), localUrl(QStringLiteral("/images/./")));
    const ImageDocumentPageCandidateListContext otherDirectoryContext
        = ImageDocumentPageCandidateListContext::forDirectory(
            localUrl(QStringLiteral("/other/01.png")), localUrl(QStringLiteral("/other/")));

    QVERIFY(KiriView::sameImageDocumentPageCandidateListSource(
        directoryContext.source(), normalizedDirectoryContext.source()));
    QVERIFY(KiriView::sameImageDocumentPageCandidateListContext(
        directoryContext, normalizedDirectoryContext));
    QVERIFY(!KiriView::sameImageDocumentPageCandidateListSource(
        directoryContext.source(), otherDirectoryContext.source()));
    QVERIFY(!KiriView::sameImageDocumentPageCandidateListContext(
        directoryContext, otherDirectoryContext));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl pageUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    const ImageDocumentPageCandidateListContext archiveContext
        = ImageDocumentPageCandidateListContext::forOpenedCollectionScope(
            pageUrl, *openedCollectionScope);

    QVERIFY(!KiriView::sameImageDocumentPageCandidateListSource(
        directoryContext.source(), archiveContext.source()));
    QVERIFY(!KiriView::sameImageDocumentPageCandidateListContext(directoryContext, archiveContext));
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateListSource)

#include "test_imagedocumentpagecandidatelistsource.moc"
