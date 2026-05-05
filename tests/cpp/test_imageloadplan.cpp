// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imageloadplan.h"

#include <QObject>
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
    void containerNavigationRestoresArchiveDocumentForInteriorImage();
    void displayedArchiveContextIsKeptForInteriorImage();
    void explicitKioArchiveImagePlansDirectLoad();
};

void TestImageLoadPlan::localFilePlansDirectImageLoad()
{
    const QUrl imageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(7, KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(plan.session.id, quint64(7));
    QVERIFY(!plan.requiresArchiveListing);
    QCOMPARE(plan.session.location.imageUrl(), imageUrl);
    QVERIFY(plan.session.location.archiveDocument().isEmpty());
}

void TestImageLoadPlan::localComicBookArchivePlansArchiveListing()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(8, KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(plan.session.id, quint64(8));
    QVERIFY(plan.requiresArchiveListing);
    QCOMPARE(plan.session.location.imageUrl(), archiveUrl);
    QCOMPARE(plan.session.location.archiveDocumentFileUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(
        plan.session.location.archiveDocument().kind(), KiriView::ArchiveDocumentKind::ComicBook);
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

    QCOMPARE(plan.session.id, quint64(9));
    QVERIFY(!plan.requiresArchiveListing);
    QCOMPARE(plan.session.location.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(plan.session.request.containerNavigationUrl(), archiveUrl);
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

    QCOMPARE(plan.session.id, quint64(10));
    QVERIFY(!plan.requiresArchiveListing);
    QCOMPARE(plan.session.location.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location.archiveDocumentFileUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadPlan::explicitKioArchiveImagePlansDirectLoad()
{
    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(11, KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(plan.session.id, quint64(11));
    QVERIFY(!plan.requiresArchiveListing);
    QCOMPARE(plan.session.location.imageUrl(), imageUrl);
    QVERIFY(plan.session.location.archiveDocument().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageLoadPlan)

#include "test_imageloadplan.moc"
