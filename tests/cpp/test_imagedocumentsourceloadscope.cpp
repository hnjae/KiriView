// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentsourceloadrequest.h"
#include "document/imagedocumentsourceloadscope.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;
}

class TestImageDocumentSourceLoadScope : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directlyOpenedArchiveResolvesOpenedCollectionScope();
    void containerNavigationRestoresScopeForInteriorImage();
    void displayedScopeIsKeptForInteriorImage();
    void ordinaryImageResolvesEmptyScope();
};

void TestImageDocumentSourceLoadScope::directlyOpenedArchiveResolvesOpenedCollectionScope()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());

    const kiriview::OpenedCollectionScopeLocation scope
        = kiriview::openedCollectionScopeForImageDocumentSourceLoad(
            kiriview::ImageDocumentSourceLoadRequest::fromUrl(archiveUrl),
            kiriview::OpenedCollectionScopeLocation::none());

    QCOMPARE(scope.fileUrl(), archiveCollection->fileUrl());
    QCOMPARE(scope.rootUrl(), archiveCollection->rootUrl());
    QCOMPARE(scope.kind(), kiriview::OpenedCollectionScopeKind::ComicBookArchive);
}

void TestImageDocumentSourceLoadScope::containerNavigationRestoresScopeForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    const kiriview::OpenedCollectionScopeLocation scope
        = kiriview::openedCollectionScopeForImageDocumentSourceLoad(
            kiriview::ImageDocumentSourceLoadRequest::fromContainerImage(imageUrl, archiveUrl),
            kiriview::OpenedCollectionScopeLocation::none());

    QCOMPARE(scope.fileUrl(), archiveCollection->fileUrl());
    QCOMPARE(scope.rootUrl(), archiveCollection->rootUrl());
}

void TestImageDocumentSourceLoadScope::displayedScopeIsKeptForInteriorImage()
{
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl imageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));

    const kiriview::OpenedCollectionScopeLocation scope
        = kiriview::openedCollectionScopeForImageDocumentSourceLoad(
            kiriview::ImageDocumentSourceLoadRequest::fromPageNavigation(imageUrl, true),
            *archiveCollection);

    QCOMPARE(scope.fileUrl(), archiveCollection->fileUrl());
    QCOMPARE(scope.rootUrl(), archiveCollection->rootUrl());
}

void TestImageDocumentSourceLoadScope::ordinaryImageResolvesEmptyScope()
{
    const kiriview::OpenedCollectionScopeLocation scope
        = kiriview::openedCollectionScopeForImageDocumentSourceLoad(
            kiriview::ImageDocumentSourceLoadRequest::fromUrl(
                localUrl(QStringLiteral("/images/01.png"))),
            kiriview::OpenedCollectionScopeLocation::none());

    QVERIFY(scope.isEmpty());
}

QTEST_GUILESS_MAIN(TestImageDocumentSourceLoadScope)

#include "test_imagedocumentsourceloadscope.moc"
