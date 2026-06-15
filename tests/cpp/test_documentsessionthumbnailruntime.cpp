// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionthumbnailruntime.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl &rootUrl, const QString &entryPath)
{
    QUrl url = rootUrl;
    url.setPath(rootUrl.path() + entryPath);
    return url;
}

kiriview::ActiveNavigationThumbnailRow thumbnailRow(int number, const QUrl &url)
{
    return kiriview::ActiveNavigationThumbnailRow {
        number,
        url,
        url.fileName(QUrl::PrettyDecoded),
        kiriview::ActiveNavigationThumbnailKind::Image,
        kiriview::ActiveNavigationThumbnailSourceKind::ImageDocumentPageImage,
        true,
    };
}

QImage testThumbnailImage()
{
    QImage image(QSize(2, 1), QImage::Format_RGBA8888);
    image.fill(Qt::cyan);
    return image;
}
}

class TestDocumentSessionThumbnailRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageDocumentPageRowsUseOpenedCollectionScopeFromLeafSnapshot();
};

void TestDocumentSessionThumbnailRuntime::
    imageDocumentPageRowsUseOpenedCollectionScopeFromLeafSnapshot()
{
    using Priority = kiriview::ActiveNavigationThumbnailDemandPriority;
    using Status = kiriview::ThumbnailGenerationStatus;

    QObject owner;
    const kiriview::OpenedCollectionScopeLocation cbzScope
        = kiriview::OpenedCollectionScopeLocation::fromUrls(
            localUrl(QStringLiteral("/books/book.cbz")),
            QUrl(QStringLiteral("zip:///books/book.cbz/")),
            kiriview::OpenedCollectionScopeKind::ComicBookArchive);
    kiriview::DocumentSessionImageDocumentSnapshot imageSnapshot;
    imageSnapshot.displayedOpenedCollectionScope = cbzScope;

    kiriview::DocumentSessionImageDocumentPort imageDocument;
    imageDocument.snapshot = [&imageSnapshot]() { return imageSnapshot; };

    auto store = std::make_shared<kiriview::ThumbnailImageStore>();
    kiriview::ThumbnailGenerationRequest generatedRequest;
    int generationCount = 0;
    kiriview::ActiveNavigationThumbnailRuntimeDependencies dependencies;
    dependencies.imageStore = store;
    dependencies.generationProvider = [&generatedRequest, &generationCount](QObject *,
                                          kiriview::ThumbnailGenerationRequest request,
                                          kiriview::ThumbnailGenerationCallback callback) {
        ++generationCount;
        generatedRequest = std::move(request);
        callback(kiriview::ThumbnailGenerationResult {
            Status::Ready,
            testThumbnailImage(),
            generatedRequest.requestedBucket,
            QStringLiteral("/cache/generated.png"),
        });
        return kiriview::ImageIoJob {};
    };
    dependencies.lookupProvider
        = [](QObject *, kiriview::ThumbnailCacheLookupRequest,
              kiriview::ThumbnailCacheLookupCallback) { return kiriview::ImageIoJob {}; };

    kiriview::DocumentSessionThumbnailRuntime runtime(
        &owner, &imageDocument, std::move(dependencies));
    const QUrl pageUrl = archivePageUrl(cbzScope.rootUrl(), QStringLiteral("pages/001.png"));
    runtime.setRows({ thumbnailRow(1, pageUrl) });

    QVERIFY(
        runtime.reportDemand(1, pageUrl, 256, Priority::Visible, runtime.navigationGeneration()));

    QCOMPARE(generationCount, 1);
    QCOMPARE(generatedRequest.openedCollectionScope, cbzScope);
    QCOMPARE(generatedRequest.sourceUrl, pageUrl);
    QCOMPARE(generatedRequest.cacheInstallEnabled, true);
    QCOMPARE(store->size(), qsizetype(1));
}

QTEST_GUILESS_MAIN(TestDocumentSessionThumbnailRuntime)

#include "test_documentsessionthumbnailruntime.moc"
