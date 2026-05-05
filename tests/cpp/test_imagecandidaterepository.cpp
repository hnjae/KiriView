// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"

#include "image_test_support.h"
#include "imagecontainer.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageCandidateRepositoryError;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::containerCandidate;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::localUrl;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;
}

class TestImageCandidateRepository : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageContextsSelectDirectoryOrArchiveListing();
    void directoryContainerOpensFirstImage();
    void archiveContainerOpensFirstImage();
    void emptyContainerReportsError();
    void listingErrorIsForwarded();
};

void TestImageCandidateRepository::displayedImageContextsSelectDirectoryOrArchiveListing()
{
    const QUrl fileUrl = localUrl(QStringLiteral("/images/02.png"));
    const std::optional<KiriView::ImageCandidateListContext> directoryContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromUrl(fileUrl));
    QVERIFY(directoryContext.has_value());
    QCOMPARE(directoryContext->currentUrl, fileUrl);
    QCOMPARE(directoryContext->listUrl, localUrl(QStringLiteral("/images/")));
    QCOMPARE(directoryContext->containerType, KiriView::ImageCandidateContainerType::Directory);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageCandidateListContext> archiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument));
    QVERIFY(archiveContext.has_value());
    QCOMPARE(archiveContext->currentUrl, pageUrl);
    QCOMPARE(archiveContext->listUrl, archiveDocument->rootUrl());
    QCOMPARE(archiveContext->archiveDocument.rootUrl(), archiveDocument->rootUrl());
    QCOMPARE(archiveContext->containerType, KiriView::ImageCandidateContainerType::ArchiveDocument);

    const QUrl directArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> directArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(directArchiveUrl);
    QVERIFY(directArchiveDocument.has_value());
    const QUrl directArchivePageUrl
        = archivePageUrl(directArchiveDocument->rootUrl(), QStringLiteral("02.png"));

    const std::optional<KiriView::ImageCandidateListContext> directArchiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromArchiveDocument(
                directArchivePageUrl, *directArchiveDocument));
    QVERIFY(directArchiveContext.has_value());
    QCOMPARE(directArchiveContext->currentUrl, directArchivePageUrl);
    QCOMPARE(directArchiveContext->listUrl, directArchiveDocument->rootUrl());
    QCOMPARE(directArchiveContext->archiveDocument.rootUrl(), directArchiveDocument->rootUrl());
    QCOMPARE(directArchiveContext->containerType,
        KiriView::ImageCandidateContainerType::ArchiveDocument);

    const QUrl explicitArchiveImageUrl(QStringLiteral("zip:///books/book.cbz/chapter/02.png"));
    const std::optional<KiriView::ImageCandidateListContext> explicitArchiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(
            KiriView::DisplayedImageLocation::fromUrl(explicitArchiveImageUrl));
    QVERIFY(explicitArchiveContext.has_value());
    QCOMPARE(explicitArchiveContext->currentUrl, explicitArchiveImageUrl);
    QCOMPARE(
        explicitArchiveContext->listUrl, QUrl(QStringLiteral("zip:///books/book.cbz/chapter/")));
    QVERIFY(explicitArchiveContext->archiveDocument.isEmpty());
    QCOMPARE(
        explicitArchiveContext->containerType, KiriView::ImageCandidateContainerType::Directory);
}

void TestImageCandidateRepository::directoryContainerOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    fakeProvider.directoryImagesByUrl[keyForUrl(containerUrl)] = { imageCandidate(imageUrl) };

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory),
        [&openedImageUrl, &openedContainerUrl](
            const QUrl &candidateImageUrl, const QUrl &candidateContainerUrl) {
            openedImageUrl = candidateImageUrl;
            openedContainerUrl = candidateContainerUrl;
        },
        {});

    QCOMPARE(openedImageUrl, imageUrl);
    QCOMPARE(openedContainerUrl, containerUrl);
}

void TestImageCandidateRepository::archiveContainerOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(containerUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.archiveImagesByUrl[keyForUrl(archiveDocument->rootUrl())] = {
        imageCandidate(imageUrl),
    };

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        [&openedImageUrl, &openedContainerUrl](
            const QUrl &candidateImageUrl, const QUrl &candidateContainerUrl) {
            openedImageUrl = candidateImageUrl;
            openedContainerUrl = candidateContainerUrl;
        },
        {});

    QCOMPARE(openedImageUrl, imageUrl);
    QCOMPARE(openedContainerUrl, containerUrl);
}

void TestImageCandidateRepository::emptyContainerReportsError()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.directoryImagesByUrl[keyForUrl(containerUrl)] = {};

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::Generic;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory), {},
        [&errorContainerUrl, &error](
            const QUrl &container, ImageCandidateRepositoryError type, const QString &) {
            errorContainerUrl = container;
            error = type;
        });

    QCOMPARE(errorContainerUrl, containerUrl);
    QCOMPARE(error, ImageCandidateRepositoryError::EmptyContainer);
}

void TestImageCandidateRepository::listingErrorIsForwarded()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.directoryImageErrorsByUrl[keyForUrl(containerUrl)] = QStringLiteral("No access");

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::EmptyContainer;
    QString errorString;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory), {},
        [&errorContainerUrl, &error, &errorString](
            const QUrl &container, ImageCandidateRepositoryError type, const QString &message) {
            errorContainerUrl = container;
            error = type;
            errorString = message;
        });

    QCOMPARE(errorContainerUrl, containerUrl);
    QCOMPARE(error, ImageCandidateRepositoryError::Generic);
    QCOMPARE(errorString, QStringLiteral("No access"));
}

QTEST_GUILESS_MAIN(TestImageCandidateRepository)

#include "test_imagecandidaterepository.moc"
