// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletion.h"

#include "image_test_support.h"
#include "imagecontainer.h"

#include <QObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;
}

class TestFileDeletion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyLocationHasNoDeletionTarget();
    void regularImageDeletionTargetsDisplayedUrl();
    void explicitKioArchiveImageDeletionTargetsDisplayedUrl();
    void directArchiveDocumentDeletionTargetsArchiveFile();
    void directDirectoryDocumentDeletionTargetsDirectory();
    void completionActionRoutesDeletionResults();
};

void TestFileDeletion::emptyLocationHasNoDeletionTarget()
{
    QVERIFY(KiriView::deletionTargetUrlForDisplayedLocation(KiriView::DisplayedImageLocation())
            .isEmpty());
}

void TestFileDeletion::regularImageDeletionTargetsDisplayedUrl()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));

    QCOMPARE(KiriView::deletionTargetUrlForDisplayedLocation(
                 KiriView::DisplayedImageLocation::fromUrl(imageUrl)),
        imageUrl);
}

void TestFileDeletion::explicitKioArchiveImageDeletionTargetsDisplayedUrl()
{
    QUrl imageUrl;
    imageUrl.setScheme(QStringLiteral("zip"));
    imageUrl.setPath(QStringLiteral("/books/book.zip/page.png"));

    QCOMPARE(KiriView::deletionTargetUrlForDisplayedLocation(
                 KiriView::DisplayedImageLocation::fromUrl(imageUrl)),
        imageUrl);
}

void TestFileDeletion::directArchiveDocumentDeletionTargetsArchiveFile()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    QCOMPARE(KiriView::deletionTargetUrlForDisplayedLocation(
                 KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument)),
        archiveUrl);
}

void TestFileDeletion::directDirectoryDocumentDeletionTargetsDirectory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = QUrl::fromLocalFile(directory.path());
    const std::optional<KiriView::ArchiveDocumentLocation> directoryDocument
        = KiriView::directOpenDocumentLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());
    QUrl pageUrl = directoryDocument->rootUrl();
    pageUrl.setPath(directoryDocument->rootUrl().path() + QStringLiteral("page.png"));

    QCOMPARE(
        KiriView::deletionTargetUrlForDisplayedLocation(
            KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *directoryDocument)),
        directoryUrl);
}

void TestFileDeletion::completionActionRoutesDeletionResults()
{
    QCOMPARE(static_cast<int>(
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Succeeded)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback));
    QCOMPARE(static_cast<int>(
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Canceled)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::Ignore));
    QCOMPARE(static_cast<int>(
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Failed)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::ReportFailure));
}

QTEST_GUILESS_MAIN(TestFileDeletion)

#include "test_filedeletion.moc"
