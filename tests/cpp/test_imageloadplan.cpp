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
    void containerNavigationRestoresArchiveCollectionForInteriorImage();
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
    QVERIFY(plan.session.openedCollectionScope().isEmpty());
}

void TestImageLoadPlan::localComicBookArchivePlansArchiveListing()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(8, KiriView::ImageLoadRequest::fromUrl(archiveUrl));

    QCOMPARE(plan.session.id(), quint64(8));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadOpenedCollectionScopeCandidates);
    QCOMPARE(plan.session.imageUrl(), archiveUrl);
    QCOMPARE(
        plan.session.location().openedCollectionScopeSourceUrl(), archiveCollection->fileUrl());
    QCOMPARE(plan.session.location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QCOMPARE(plan.session.openedCollectionScope().kind(),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}

void TestImageLoadPlan::localDirectoryPlansDocumentListing()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<KiriView::OpenedCollectionScopeLocation> directoryCollection
        = KiriView::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());

    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(12, KiriView::ImageLoadRequest::fromUrl(directoryUrl));

    QCOMPARE(plan.session.id(), quint64(12));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::LoadOpenedCollectionScopeCandidates);
    QCOMPARE(plan.session.imageUrl(), directoryUrl);
    QCOMPARE(
        plan.session.location().openedCollectionScopeSourceUrl(), directoryCollection->fileUrl());
    QCOMPARE(
        plan.session.location().openedCollectionScopeRootUrl(), directoryCollection->rootUrl());
    QCOMPARE(plan.session.openedCollectionScope().kind(),
        KiriView::OpenedCollectionScopeKind::Directory);
}

void TestImageLoadPlan::containerNavigationRestoresArchiveCollectionForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(9,
        KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::OpenedCollectionScopeLocation::none(), archiveUrl));
    const KiriView::OpenedCollectionScopeLoadPlan archivePlan
        = KiriView::openedCollectionScopeLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            imageUrl, KiriView::OpenedCollectionScopeLocation::none(), archiveUrl));

    QCOMPARE(plan.session.id(), quint64(9));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(plan.session.location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QCOMPARE(plan.session.containerNavigationUrl(), archiveUrl);
    QCOMPARE(archivePlan.effect, KiriView::OpenedCollectionScopeLoadEffect::ReadImage);
    QCOMPARE(archivePlan.openedCollectionScope.rootUrl(), archiveCollection->rootUrl());
}

void TestImageLoadPlan::displayedArchiveContextIsKeptForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page.png"));

    const KiriView::ImageLoadPlan plan = KiriView::imageLoadPlan(
        10, KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));
    const KiriView::OpenedCollectionScopeLoadPlan archivePlan
        = KiriView::openedCollectionScopeLoadPlan(
            KiriView::ImageLoadRequest::fromLocation(imageUrl, *archiveCollection));

    QCOMPARE(plan.session.id(), quint64(10));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QCOMPARE(
        plan.session.location().openedCollectionScopeSourceUrl(), archiveCollection->fileUrl());
    QCOMPARE(plan.session.location().openedCollectionScopeRootUrl(), archiveCollection->rootUrl());
    QCOMPARE(archivePlan.effect, KiriView::OpenedCollectionScopeLoadEffect::ReadImage);
    QCOMPARE(archivePlan.openedCollectionScope.fileUrl(), archiveCollection->fileUrl());
    QCOMPARE(archivePlan.openedCollectionScope.rootUrl(), archiveCollection->rootUrl());
}

void TestImageLoadPlan::explicitKioArchiveImagePlansDirectLoad()
{
    const QUrl imageUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    const KiriView::ImageLoadPlan plan
        = KiriView::imageLoadPlan(11, KiriView::ImageLoadRequest::fromUrl(imageUrl));

    QCOMPARE(plan.session.id(), quint64(11));
    QCOMPARE(plan.startEffect, KiriView::ImageLoadStartEffect::DecodeImage);
    QCOMPARE(plan.session.imageUrl(), imageUrl);
    QVERIFY(plan.session.openedCollectionScope().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageLoadPlan)

#include "test_imageloadplan.moc"
