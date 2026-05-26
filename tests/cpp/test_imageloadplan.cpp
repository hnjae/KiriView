// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imageloadplan.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

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
    QVERIFY(plan.session.imagePageScope().isEmpty());
}

void TestImageLoadPlan::localComicBookArchivePlansArchiveListing()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(8, KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(plan.session.id(), quint64(8));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadImagePageScopeCandidates);
    QCOMPARE(plan.session.imageUrl(), archiveUrl);
    QCOMPARE(plan.session.location().imagePageScopeSourceUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location().imagePageScopeRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(plan.session.imagePageScope().kind(), KiriView::ImagePageScopeKind::ComicBookArchive);
}

void TestImageLoadPlan::localDirectoryPlansDocumentListing()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::ImagePageScopeLocation> directoryDocument
        = KiriView::directOpenImagePageScopeLocationForLocalUrl(directoryUrl);
    QVERIFY(directoryDocument.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(12, KiriView::ImageLoadRequest::fromUrl(directoryUrl));

    QCOMPARE(plan.session.id(), quint64(12));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadImagePageScopeCandidates);
    QCOMPARE(plan.session.imageUrl(), directoryUrl);
    QCOMPARE(plan.session.location().imagePageScopeSourceUrl(), directoryDocument->fileUrl());
    QCOMPARE(plan.session.location().imagePageScopeRootUrl(), directoryDocument->rootUrl());
    QCOMPARE(plan.session.imagePageScope().kind(), KiriView::ImagePageScopeKind::Directory);
}

void TestImageLoadPlan::containerNavigationRestoresArchiveDocumentForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(9,
        KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::ImagePageScopeLocation::none(), archiveUrl));
    const KiriView::ImagePageScopeLoadPlan archivePlan
        = KiriView::imagePageScopeLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::ImagePageScopeLocation::none(), archiveUrl));

    QCOMPARE(plan.session.id(), quint64(9));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location().imagePageScopeRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(plan.session.containerNavigationUrl(), archiveUrl);
    QCOMPARE(archivePlan.effect, KiriView::ImagePageScopeLoadEffect::ReadImage);
    QCOMPARE(archivePlan.imagePageScope.rootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadPlan::displayedArchiveContextIsKeptForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl imageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(
        10, KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveDocument));
    const KiriView::ImagePageScopeLoadPlan archivePlan = KiriView::imagePageScopeLoadPlan(
        KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveDocument));

    QCOMPARE(plan.session.id(), quint64(10));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location().imagePageScopeSourceUrl(), archiveDocument->fileUrl());
    QCOMPARE(plan.session.location().imagePageScopeRootUrl(), archiveDocument->rootUrl());
    QCOMPARE(archivePlan.effect, KiriView::ImagePageScopeLoadEffect::ReadImage);
    QCOMPARE(archivePlan.imagePageScope.fileUrl(), archiveDocument->fileUrl());
    QCOMPARE(archivePlan.imagePageScope.rootUrl(), archiveDocument->rootUrl());
}

void TestImageLoadPlan::explicitKioArchiveImagePlansDirectLoad()
{
    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(11, KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(plan.session.id(), quint64(11));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QVERIFY(plan.session.imagePageScope().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageLoadPlan)

#include "test_imageloadplan.moc"
