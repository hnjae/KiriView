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
    void archiveCollectionIdentityComparesNormalizedUrlsAndKind();
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

    const KiriView::OpenedCollectionScopeLocation archiveCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            rawImageUrl, rawArchiveRootUrl, KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(archiveCollection.fileUrl(), normalizedImageUrl);
    QCOMPARE(archiveCollection.rootUrl(), normalizedArchiveRootUrl);
    QVERIFY(archiveCollection
        == KiriView::OpenedCollectionScopeLocation::fromUrls(normalizedImageUrl,
            normalizedArchiveRootUrl, KiriView::OpenedCollectionScopeKind::ComicBookArchive));

    const KiriView::DisplayedImageLocation displayedLocation
        = KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            rawImageUrl, archiveCollection);
    QCOMPARE(displayedLocation.imageUrl(), normalizedImageUrl);
    QVERIFY(displayedLocation
        == KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            normalizedImageUrl, archiveCollection));
}

void TestImageLocation::archiveCollectionIdentityComparesNormalizedUrlsAndKind()
{
    const KiriView::OpenedCollectionScopeLocation archiveCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            KiriView::TestSupport::localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/book.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const KiriView::OpenedCollectionScopeLocation normalizedArchiveCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(
            KiriView::TestSupport::localUrl(QStringLiteral("/books/./book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/./book.cbz/")),
            KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    const KiriView::OpenedCollectionScopeLocation differentKind
        = KiriView::OpenedCollectionScopeLocation::fromUrls(archiveCollection.fileUrl(),
            archiveCollection.rootUrl(), KiriView::OpenedCollectionScopeKind::GeneralArchive);

    QVERIFY(KiriView::sameOpenedCollectionScopeLocation(
        archiveCollection, normalizedArchiveCollection));
    QVERIFY(!KiriView::sameOpenedCollectionScopeLocation(archiveCollection, differentKind));
}

QTEST_GUILESS_MAIN(TestImageLocation)

#include "test_imagelocation.moc"
