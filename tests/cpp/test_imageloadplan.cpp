// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageloadplan.h"
#include "image_test_support.h"
#include "imagecontainer.h"

#include <QObject>
#include <QSize>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <optional>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;
}

class TestImageLoadPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localFilePlansDirectImageLoad();
    void localComicBookArchivePlansArchiveListing();
    void localDirectoryPlansDocumentListing();
    void containerNavigationRestoresArchiveDocumentForInteriorImage();
    void displayedArchiveContextIsKeptForInteriorImage();
    void explicitKioArchiveImagePlansDirectLoad();
};

void TestImageLoadPlan::localFilePlansDirectImageLoad()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(7, KiriView::ImageLoadRequest::fromUrl(imageUrl),
            KiriView::ImageFirstDisplayDecodeContext { QSize(320, 240) });

    QCOMPARE(plan.session.id(), quint64(7));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.firstDisplay().physicalViewportSize, QSize(320, 240));
    QVERIFY(plan.session.archiveDocument().isEmpty());
}

void TestImageLoadPlan::localComicBookArchivePlansArchiveListing()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(8, KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(plan.session.id(), quint64(8));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadArchiveImageCandidates);
    QCOMPARE(plan.session.imageUrl(), archiveUrl);
    QCOMPARE(plan.session.location().archiveDocumentFileUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location().archiveDocumentRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(plan.session.archiveDocument().kind(), KiriView::ArchiveDocumentKind::ComicBook);
}

void TestImageLoadPlan::localDirectoryPlansDocumentListing()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::ArchiveDocumentLocation> directoryDocument
        = KiriView::directOpenDocumentLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(12, KiriView::ImageLoadRequest::fromUrl(directoryUrl));

    QCOMPARE(plan.session.id(), quint64(12));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadArchiveImageCandidates);
    QCOMPARE(plan.session.imageUrl(), directoryUrl);
    QCOMPARE(plan.session.location().archiveDocumentFileUrl(), directoryDocument->fileUrl());
    QCOMPARE(plan.session.location().archiveDocumentRootUrl(), directoryDocument->rootUrl());
    QCOMPARE(plan.session.archiveDocument().kind(), KiriView::ArchiveDocumentKind::Directory);
}

void TestImageLoadPlan::containerNavigationRestoresArchiveDocumentForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(9,
        KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::ArchiveDocumentLocation::none(), archiveUrl));
    const KiriView::ImageArchiveLoadPlan archivePlan
        = KiriView::imageArchiveLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::ArchiveDocumentLocation::none(), archiveUrl));

    QCOMPARE(plan.session.id(), quint64(9));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location().archiveDocumentRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(plan.session.containerNavigationUrl(), archiveUrl);
    QCOMPARE(archivePlan.effect, KiriView::ImageArchiveLoadEffect::ReadImage);
    QCOMPARE(archivePlan.archiveDocument.rootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadPlan::displayedArchiveContextIsKeptForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(
        10, KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveDocument));
    const KiriView::ImageArchiveLoadPlan archivePlan = KiriView::imageArchiveLoadPlan(
        KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveDocument));

    QCOMPARE(plan.session.id(), quint64(10));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location().archiveDocumentFileUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location().archiveDocumentRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(archivePlan.effect, KiriView::ImageArchiveLoadEffect::ReadImage);
    QCOMPARE(archivePlan.archiveDocument.fileUrl(), archiveDocument->fileUrl());
    QCOMPARE(archivePlan.archiveDocument.rootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadPlan::explicitKioArchiveImagePlansDirectLoad()
{
    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(11, KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(plan.session.id(), quint64(11));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QVERIFY(plan.session.archiveDocument().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageLoadPlan)

#include "test_imageloadplan.moc"
