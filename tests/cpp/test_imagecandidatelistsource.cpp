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

using ArchiveCandidateContext = ImageCandidateListContext::ImagePageScopeContext;
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
    QVERIFY(directoryContext->imagePageScope().isEmpty());

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> imagePageScope
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(imagePageScope.has_value());
    const QUrl pageUrl = archivePageUrl(imagePageScope->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageCandidateListContext> archiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromImagePageScope(pageUrl, *imagePageScope));
    QVERIFY(archiveContext.has_value());
    QCOMPARE(archiveContext->currentUrl(), pageUrl);
    const ArchiveCandidateContext *archive
        = typedCandidateContext<ArchiveCandidateContext>(*archiveContext);
    QVERIFY(archive != nullptr);
    QCOMPARE(archive->imagePageScope.rootUrl(), imagePageScope->rootUrl());
    QCOMPARE(archiveContext->imagePageScope().rootUrl(), imagePageScope->rootUrl());

    const QUrl directArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ImagePageScopeLocation> directArchiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(directArchiveUrl);
    QVERIFY(directArchiveDocument.has_value());
    const QUrl directArchivePageUrl
        = archivePageUrl(directArchiveDocument->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageCandidateListContext> directArchiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromImagePageScope(
                directArchivePageUrl, *directArchiveDocument));
    QVERIFY(directArchiveContext.has_value());
    QCOMPARE(directArchiveContext->currentUrl(), directArchivePageUrl);
    const ArchiveCandidateContext *directArchive
        = typedCandidateContext<ArchiveCandidateContext>(*directArchiveContext);
    QVERIFY(directArchive != nullptr);
    QCOMPARE(directArchive->imagePageScope.rootUrl(), directArchiveDocument->rootUrl());
    QCOMPARE(directArchiveContext->imagePageScope().rootUrl(), directArchiveDocument->rootUrl());

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
    const std::optional<KiriView::ImagePageScopeLocation> imagePageScope
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(imagePageScope.has_value());
    const QUrl pageUrl = archivePageUrl(imagePageScope->rootUrl(), QStringLiteral("01.png"));
    const ImageCandidateListContext archiveContext
        = ImageCandidateListContext::forImagePageScope(pageUrl, *imagePageScope);

    QVERIFY(!KiriView::sameImageCandidateListSource(
        directoryContext.source(), archiveContext.source()));
    QVERIFY(!KiriView::sameImageCandidateListContext(directoryContext, archiveContext));
}

QTEST_GUILESS_MAIN(TestImageCandidateListSource)

#include "test_imagecandidatelistsource.moc"
