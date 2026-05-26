// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedocument.h"

#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QByteArray>
#include <QObject>
#include <QSignalSpy>
#include <QSizeF>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <optional>

class TestKiriImageDocument : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void archiveOrDirectoryDocumentScopeActiveFollowsDisplayedLocation();
};

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDocumentRuntimeDependencyOverridesFor;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

std::unique_ptr<KiriImageDocument> createDocument(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    auto document = std::make_unique<KiriImageDocument>(
        imageDocumentRuntimeDependencyOverridesFor(
            candidateProvider, dataLoader, staticImageDataDecoder()),
        parent);
    document->setViewportSize(QSizeF(400.0, 300.0));
    return document;
}

void loadReady(KiriImageDocument &document, ManualImageDataLoader &dataLoader,
    const QUrl &sourceUrl, const QUrl &loadUrl)
{
    QSignalSpy scopeSpy(&document, &KiriImageDocument::mediaScopeChanged);

    document.setSourceUrl(sourceUrl);
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(loadUrl, QByteArrayLiteral("ok")));

    QTRY_COMPARE(document.status(), KiriImageDocument::Status::Ready);
    QVERIFY(scopeSpy.count() > 0);
}
}

void TestKiriImageDocument::archiveOrDirectoryDocumentScopeActiveFollowsDisplayedLocation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;

    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(
        localUrl(QStringLiteral("/images/")), { imageCandidate(imageUrl) });
    std::unique_ptr<KiriImageDocument> directImageDocument
        = createDocument(this, candidateProvider, dataLoader);

    QVERIFY(!directImageDocument->archiveOrDirectoryDocumentScopeActive());
    loadReady(*directImageDocument, dataLoader, imageUrl, imageUrl);
    QVERIFY(!directImageDocument->archiveOrDirectoryDocumentScopeActive());

    const QUrl comicArchiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> comicArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(comicArchiveUrl);
    QVERIFY(comicArchiveDocument.has_value());
    const QUrl comicArchivePage
        = archivePageUrl(comicArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(
        comicArchiveDocument->rootUrl(), { imageCandidate(comicArchivePage) });
    std::unique_ptr<KiriImageDocument> comicDocument
        = createDocument(this, candidateProvider, dataLoader);

    loadReady(*comicDocument, dataLoader, comicArchiveUrl, comicArchivePage);
    QVERIFY(comicDocument->archiveOrDirectoryDocumentScopeActive());

    const QUrl generalArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> generalArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(generalArchiveUrl);
    QVERIFY(generalArchiveDocument.has_value());
    const QUrl generalArchivePage
        = archivePageUrl(generalArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(
        generalArchiveDocument->rootUrl(), { imageCandidate(generalArchivePage) });
    std::unique_ptr<KiriImageDocument> generalDocument
        = createDocument(this, candidateProvider, dataLoader);

    loadReady(*generalDocument, dataLoader, generalArchiveUrl, generalArchivePage);
    QVERIFY(generalDocument->archiveOrDirectoryDocumentScopeActive());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::ArchiveDocumentLocation> directoryDocument
        = KiriView::directOpenDocumentLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());
    const QUrl directoryPage
        = archivePageUrl(directoryDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(
        directoryDocument->rootUrl(), { imageCandidate(directoryPage) });
    std::unique_ptr<KiriImageDocument> openedDirectoryDocument
        = createDocument(this, candidateProvider, dataLoader);

    loadReady(*openedDirectoryDocument, dataLoader, directoryUrl, directoryPage);
    QVERIFY(openedDirectoryDocument->archiveOrDirectoryDocumentScopeActive());

    const QUrl archiveEntryUrl(QStringLiteral("zip:///books/book.zip!/page.png"));
    candidateProvider.setDirectoryImages(
        QUrl(QStringLiteral("zip:///books/book.zip!/")), { imageCandidate(archiveEntryUrl) });
    std::unique_ptr<KiriImageDocument> archiveEntryDocument
        = createDocument(this, candidateProvider, dataLoader);

    loadReady(*archiveEntryDocument, dataLoader, archiveEntryUrl, archiveEntryUrl);
    QVERIFY(!archiveEntryDocument->archiveOrDirectoryDocumentScopeActive());
}

QTEST_GUILESS_MAIN(TestKiriImageDocument)

#include "test_kiriimagedocument.moc"
