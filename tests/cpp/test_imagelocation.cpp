// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/imagelocation.h"

#include "candidate_test_support.h"

#include <QObject>
#include <QTest>

class TestImageLocation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void locationValuesStoreCanonicalUrlIdentity();
    void archiveDocumentIdentityComparesNormalizedUrlsAndKind();
};

void TestImageLocation::locationValuesStoreCanonicalUrlIdentity()
{
    const QUrl rawImageUrl
        = KiriView::TestSupport::localUrl(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedImageUrl
        = KiriView::TestSupport::localUrl(QStringLiteral("/images/page.png"));
    const QUrl rawArchiveRootUrl(QStringLiteral("zip:///books/./book.cbz/"));
    const QUrl normalizedArchiveRootUrl(QStringLiteral("zip:///books/book.cbz/"));

    const KiriView::ImageLocation imageLocation = KiriView::ImageLocation::fromUrl(rawImageUrl);
    QCOMPARE(imageLocation.url(), normalizedImageUrl);
    QVERIFY(imageLocation == KiriView::ImageLocation::fromUrl(normalizedImageUrl));

    const KiriView::ContainerLocation containerLocation
        = KiriView::ContainerLocation::fromUrl(rawImageUrl);
    QCOMPARE(containerLocation.url(), normalizedImageUrl);
    QVERIFY(containerLocation == KiriView::ContainerLocation::fromUrl(normalizedImageUrl));
    QVERIFY(KiriView::ContainerLocation::none() == KiriView::ContainerLocation::none());

    const KiriView::ArchiveDocumentLocation archiveDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            rawImageUrl, rawArchiveRootUrl, KiriView::ArchiveDocumentKind::ComicBook);
    QCOMPARE(archiveDocument.fileUrl(), normalizedImageUrl);
    QCOMPARE(archiveDocument.rootUrl(), normalizedArchiveRootUrl);
    QVERIFY(archiveDocument
        == KiriView::ArchiveDocumentLocation::fromUrls(normalizedImageUrl, normalizedArchiveRootUrl,
            KiriView::ArchiveDocumentKind::ComicBook));

    const KiriView::DisplayedImageLocation displayedLocation
        = KiriView::DisplayedImageLocation::fromArchiveDocument(rawImageUrl, archiveDocument);
    QCOMPARE(displayedLocation.imageUrl(), normalizedImageUrl);
    QVERIFY(displayedLocation
        == KiriView::DisplayedImageLocation::fromArchiveDocument(
            normalizedImageUrl, archiveDocument));
}

void TestImageLocation::archiveDocumentIdentityComparesNormalizedUrlsAndKind()
{
    const KiriView::ArchiveDocumentLocation archiveDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            KiriView::TestSupport::localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/book.cbz/")),
            KiriView::ArchiveDocumentKind::ComicBook);
    const KiriView::ArchiveDocumentLocation normalizedArchiveDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            KiriView::TestSupport::localUrl(QStringLiteral("/books/./book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/./book.cbz/")),
            KiriView::ArchiveDocumentKind::ComicBook);
    const KiriView::ArchiveDocumentLocation differentKind
        = KiriView::ArchiveDocumentLocation::fromUrls(archiveDocument.fileUrl(),
            archiveDocument.rootUrl(), KiriView::ArchiveDocumentKind::General);

    QVERIFY(KiriView::sameArchiveDocumentLocation(archiveDocument, normalizedArchiveDocument));
    QVERIFY(!KiriView::sameArchiveDocumentLocation(archiveDocument, differentKind));
}

QTEST_GUILESS_MAIN(TestImageLocation)

#include "test_imagelocation.moc"
