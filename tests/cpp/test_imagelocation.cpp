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
    void archiveDocumentIdentityComparesNormalizedUrlsAndKind();
};

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
