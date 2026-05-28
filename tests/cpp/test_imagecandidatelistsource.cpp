// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecandidatelistsource.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <type_traits>

namespace {
using KiriView::ImageCandidateListContext;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;

using OpenedCollectionCandidateContext = ImageCandidateListContext::OpenedCollectionScopeContext;
using DirectoryCandidateContext = ImageCandidateListContext::DirectoryContext;

template <typename ExpectedContext>
const ExpectedContext *typedCandidateContext(const ImageCandidateListContext &context)
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

class TestImageCandidateListSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageContextsSelectDirectoryOrArchiveListing();
    void candidateListIdentityComparesNormalizedSourcesAndCurrentUrls();
};

void TestImageCandidateListSource::displayedImageContextsSelectDirectoryOrArchiveListing()
{
    const QUrl fileUrl = localUrl(QStringLiteral("/images/02.png"));
    const std::optional<KiriView::ImageCandidateListContext> directoryContext
        = KiriView::imageCandidateListContextForDisplayedImage(
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

    const std::optional<KiriView::ImageCandidateListContext> archiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
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

    const std::optional<KiriView::ImageCandidateListContext> directArchiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
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
    const std::optional<KiriView::ImageCandidateListContext> explicitArchiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromUrl(explicitArchiveImageUrl));
    QVERIFY(explicitArchiveContext.has_value());
    QCOMPARE(explicitArchiveContext->currentUrl(), explicitArchiveImageUrl);
    const DirectoryCandidateContext *explicitArchiveDirectory
        = typedCandidateContext<DirectoryCandidateContext>(*explicitArchiveContext);
    QVERIFY(explicitArchiveDirectory != nullptr);
    QCOMPARE(explicitArchiveDirectory->directoryUrl,
        QUrl(QStringLiteral("zip:///books/book.cbz/chapter/")));
}

void TestImageCandidateListSource::candidateListIdentityComparesNormalizedSourcesAndCurrentUrls()
{
    const ImageCandidateListContext directoryContext = ImageCandidateListContext::forDirectory(
        localUrl(QStringLiteral("/images/01.png")), localUrl(QStringLiteral("/images/")));
    const ImageCandidateListContext normalizedDirectoryContext
        = ImageCandidateListContext::forDirectory(
            localUrl(QStringLiteral("/images/./01.png")), localUrl(QStringLiteral("/images/./")));
    const ImageCandidateListContext otherDirectoryContext = ImageCandidateListContext::forDirectory(
        localUrl(QStringLiteral("/other/01.png")), localUrl(QStringLiteral("/other/")));

    QVERIFY(KiriView::sameImageCandidateListSource(
        directoryContext.source(), normalizedDirectoryContext.source()));
    QVERIFY(KiriView::sameImageCandidateListContext(directoryContext, normalizedDirectoryContext));
    QVERIFY(!KiriView::sameImageCandidateListSource(
        directoryContext.source(), otherDirectoryContext.source()));
    QVERIFY(!KiriView::sameImageCandidateListContext(directoryContext, otherDirectoryContext));

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl pageUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    const ImageCandidateListContext archiveContext
        = ImageCandidateListContext::forOpenedCollectionScope(pageUrl, *openedCollectionScope);

    QVERIFY(!KiriView::sameImageCandidateListSource(
        directoryContext.source(), archiveContext.source()));
    QVERIFY(!KiriView::sameImageCandidateListContext(directoryContext, archiveContext));
}

QTEST_GUILESS_MAIN(TestImageCandidateListSource)

#include "test_imagecandidatelistsource.moc"
