// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "thumbnail/thumbnailgeneration.h"

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QImageWriter>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using Bucket = KiriView::ActiveNavigationThumbnailDemandBucket;
using Status = KiriView::ThumbnailGenerationStatus;

QByteArray encodedPngData()
{
    QImage image(QSize(4, 3), QImage::Format_RGBA8888);
    image.fill(QColor(Qt::green));

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, QByteArrayLiteral("png"));
    if (!writer.write(image)) {
        return {};
    }
    return data;
}

KiriView::ThumbnailGenerationRequest generationRequest(Bucket bucket = Bucket::Normal)
{
    KiriView::ThumbnailGenerationRequest request;
    request.localPathBytes = QByteArrayLiteral("/missing/source.png");
    request.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/missing/source.png"));
    request.sourceLabel = QStringLiteral("source.png");
    request.requestedBucket = bucket;
    request.cacheInstallEnabled = false;
    return request;
}
}

class TestThumbnailGeneration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void injectedBytesLoaderProvidesGenerationBytes();
    void openedCollectionIdentityFailureSkipsBytesLoader();
};

void TestThumbnailGeneration::injectedBytesLoaderProvidesGenerationBytes()
{
    KiriView::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const KiriView::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(generationRequest(), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 1);
    QCOMPARE(result.status, Status::Ready);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.image.size(), QSize(4, 3));
    QCOMPARE(result.image.format(), QImage::Format_RGBA8888);
    QVERIFY(result.installedCachePath.isEmpty());
}

void TestThumbnailGeneration::openedCollectionIdentityFailureSkipsBytesLoader()
{
    KiriView::ThumbnailGenerationRequest request = generationRequest();
    request.openedCollectionScope = KiriView::OpenedCollectionScopeLocation::fromUrls(
        QUrl::fromLocalFile(QStringLiteral("/books/book.cbz")),
        QUrl(QStringLiteral("zip:///books/book.cbz")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
    request.sourceUrl = QUrl(QStringLiteral("zip:///books/book.cbz/page.png"));
    request.cacheInstallEnabled = true;

    KiriView::ThumbnailGenerationDependencies dependencies;
    int bytesLoadCount = 0;
    dependencies.bytesLoader
        = [&bytesLoadCount](const KiriView::ThumbnailGenerationRequest &, QString *) {
              ++bytesLoadCount;
              return encodedPngData();
          };
    dependencies.openedCollectionOriginalIdentityLoader
        = [](const KiriView::ThumbnailGenerationRequest &, QString *errorString) {
              if (errorString != nullptr) {
                  *errorString = QStringLiteral("synthetic identity failure");
              }
              return std::optional<KiriView::ThumbnailOriginalIdentity>();
          };

    const KiriView::ThumbnailGenerationResult result
        = KiriView::generateThumbnail(std::move(request), std::move(dependencies));

    QCOMPARE(bytesLoadCount, 0);
    QCOMPARE(result.status, Status::Failed);
    QCOMPARE(result.requestedBucket, Bucket::Normal);
    QCOMPARE(result.errorString, QStringLiteral("synthetic identity failure"));
}

QTEST_GUILESS_MAIN(TestThumbnailGeneration)

#include "test_thumbnailgeneration.moc"
