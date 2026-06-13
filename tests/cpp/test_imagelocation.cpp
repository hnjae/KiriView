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
        = kiriview::TestSupport::localUrl(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedImageUrl
        = kiriview::TestSupport::localUrl(QStringLiteral("/images/page.png"));
    const QUrl rawArchiveRootUrl(QStringLiteral("zip:///books/./book.cbz/"));
    const QUrl normalizedArchiveRootUrl(QStringLiteral("zip:///books/book.cbz/"));

    const kiriview::ImageLocation imageLocation = kiriview::ImageLocation::fromUrl(rawImageUrl);
    QCOMPARE(imageLocation.url(), normalizedImageUrl);
    QVERIFY(imageLocation == kiriview::ImageLocation::fromUrl(normalizedImageUrl));

    const kiriview::ContainerLocation containerLocation
        = kiriview::ContainerLocation::fromUrl(rawImageUrl);
    QCOMPARE(containerLocation.url(), normalizedImageUrl);
    QVERIFY(containerLocation == kiriview::ContainerLocation::fromUrl(normalizedImageUrl));
    QVERIFY(kiriview::ContainerLocation::none() == kiriview::ContainerLocation::none());

    const kiriview::OpenedCollectionScopeLocation archiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            rawImageUrl, rawArchiveRootUrl, kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    QCOMPARE(archiveCollection.fileUrl(), normalizedImageUrl);
    QCOMPARE(archiveCollection.rootUrl(), normalizedArchiveRootUrl);
    QVERIFY(archiveCollection
        == kiriview::OpenedCollectionScopeLocation::fromUrls(normalizedImageUrl,
            normalizedArchiveRootUrl, kiriview::OpenedCollectionScopeKind::ComicBookArchive));

    const kiriview::DisplayedImageLocation displayedLocation
        = kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            rawImageUrl, archiveCollection);
    QCOMPARE(displayedLocation.imageUrl(), normalizedImageUrl);
    QVERIFY(displayedLocation
        == kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            normalizedImageUrl, archiveCollection));
}

void TestImageLocation::archiveCollectionIdentityComparesNormalizedUrlsAndKind()
{
    const kiriview::OpenedCollectionScopeLocation archiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            kiriview::TestSupport::localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const kiriview::OpenedCollectionScopeLocation normalizedArchiveCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            kiriview::TestSupport::localUrl(QStringLiteral("/books/./book.cbz")),
            QUrl(QStringLiteral("kio-fuse:///books/./book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    const kiriview::OpenedCollectionScopeLocation differentKind
        = kiriview::OpenedCollectionScopeLocation::fromUrls(archiveCollection.fileUrl(),
            archiveCollection.rootUrl(), kiriview::OpenedCollectionScopeKind::GeneralArchive);

    QVERIFY(kiriview::sameOpenedCollectionScopeLocation(
        archiveCollection, normalizedArchiveCollection));
    QVERIFY(!kiriview::sameOpenedCollectionScopeLocation(archiveCollection, differentKind));
}

QTEST_GUILESS_MAIN(TestImageLocation)

#include "test_imagelocation.moc"
