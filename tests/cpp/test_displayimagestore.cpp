// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/displayimagestore.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <QUrl>
#include <QtGlobal>
#include <future>
#include <optional>
#include <vector>

class TestDisplayImageStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storesImagesAndMetadata();
    void urlShapeUsesNeverReusedIds();
    void providerReportsOriginalSizeAndMissesReturnEmpty();
    void providerHandlesRequestedSizeAsDownscaleOnly();
    void evictsLeastRecentlyUsedImagesByPriority();
    void pinLeasesPreventEvictionAndReleaseDefersRemoval();
    void providerRequestsAreThreadSafeReads();
};

namespace {
QImage testImage(const QSize& size, QColor color = Qt::red)
{
    QImage image(size, QImage::Format_RGBA8888);
    image.fill(color);
    return image;
}

kiriview::DisplayImageEntry testEntry(
    const QSize& rasterSize, kiriview::DisplayImageRetentionPriority priority = {})
{
    return kiriview::DisplayImageEntry {
        testImage(rasterSize),
        QSize(rasterSize.width() * 2, rasterSize.height() * 2),
        rasterSize,
        QStringLiteral("file:///tmp/image.png"),
        kiriview::DisplayedPageRole::Primary,
        kiriview::DisplayImageQuality::Exact,
        priority,
        42,
        QStringLiteral("test-entry"),
    };
}
}

void TestDisplayImageStore::storesImagesAndMetadata()
{
    kiriview::DisplayImageStore store(1024);

    const QString id = store.insert(testEntry(QSize(8, 4)));
    QVERIFY(!id.isEmpty());

    const std::optional<kiriview::DisplayImageStoreEntry> stored = store.entry(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id, id);
    QCOMPARE(stored->image.size(), QSize(8, 4));
    QCOMPARE(stored->originalSize, QSize(16, 8));
    QCOMPARE(stored->rasterSize, QSize(8, 4));
    QCOMPARE(stored->sourceIdentity, QStringLiteral("file:///tmp/image.png"));
    QCOMPARE(stored->pageRole, kiriview::DisplayedPageRole::Primary);
    QCOMPARE(stored->quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(stored->priority, kiriview::DisplayImageRetentionPriority::Nearby);
    QCOMPARE(stored->generation, quint64(42));
    QCOMPARE(stored->debugLabel, QStringLiteral("test-entry"));
    QCOMPARE(stored->byteCost, qsizetype(128));
    QCOMPARE(store.byteCost(), qsizetype(128));
    QCOMPARE(store.size(), qsizetype(1));
}

void TestDisplayImageStore::urlShapeUsesNeverReusedIds()
{
    kiriview::DisplayImageStore store(1024);

    const QString first = store.insert(testEntry(QSize(4, 4)));
    store.release(first);
    const QString second = store.insert(testEntry(QSize(4, 4)));

    QVERIFY(!first.isEmpty());
    QVERIFY(!second.isEmpty());
    QVERIFY(first != second);
    QCOMPARE(kiriview::displayImageSourceForId(second),
        QUrl(QStringLiteral("image://kiriview-images/%1").arg(second)));
    QVERIFY(kiriview::displayImageSourceForId({}).isEmpty());
}

void TestDisplayImageStore::providerReportsOriginalSizeAndMissesReturnEmpty()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(1024);
    const QString id = store->insert(testEntry(QSize(8, 4)));
    kiriview::DisplayImageProvider provider(store);

    QSize originalSize;
    const QImage image = provider.requestImage(id, &originalSize, {});
    QCOMPARE(image.size(), QSize(8, 4));
    QCOMPARE(originalSize, QSize(16, 8));

    QSize missSize(1, 1);
    QVERIFY(provider.requestImage(QStringLiteral("missing"), &missSize, {}).isNull());
    QCOMPARE(missSize, QSize());
}

void TestDisplayImageStore::providerHandlesRequestedSizeAsDownscaleOnly()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(32768);
    const QString id = store->insert(testEntry(QSize(80, 40)));
    kiriview::DisplayImageProvider provider(store);

    QSize originalSize;
    QCOMPARE(provider.requestImage(id, &originalSize, QSize(40, 40)).size(), QSize(40, 20));
    QCOMPARE(originalSize, QSize(160, 80));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(20, 0)).size(), QSize(20, 10));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(0, 10)).size(), QSize(20, 10));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(0, 0)).size(), QSize(80, 40));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(-20, 10)).size(), QSize(80, 40));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(160, 80)).size(), QSize(80, 40));
    QCOMPARE(provider.requestImage(id, nullptr, QSize(80, 40)).size(), QSize(80, 40));
}

void TestDisplayImageStore::evictsLeastRecentlyUsedImagesByPriority()
{
    kiriview::DisplayImageStore store(128);

    const QString background
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Background));
    const QString nearby
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Nearby));
    const QString visible
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Visible));

    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(!background.isEmpty());
    QVERIFY(store.entry(background) == std::nullopt);
    QVERIFY(store.entry(nearby).has_value());
    QVERIFY(store.entry(visible).has_value());
}

void TestDisplayImageStore::pinLeasesPreventEvictionAndReleaseDefersRemoval()
{
    kiriview::DisplayImageStore store(128);

    const QString pinned
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Background));
    QVERIFY(store.acquirePinLease(pinned, kiriview::DisplayImagePinKind::Visible));
    const QString newer
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Background));
    const QString newest
        = store.insert(testEntry(QSize(4, 4), kiriview::DisplayImageRetentionPriority::Background));

    QCOMPARE(store.size(), qsizetype(2));
    QVERIFY(store.entry(pinned).has_value());
    QVERIFY(store.entry(newer) == std::nullopt);
    QVERIFY(store.entry(newest).has_value());

    store.release(pinned);
    QVERIFY(store.entry(pinned).has_value());
    store.releasePinLease(pinned, kiriview::DisplayImagePinKind::Visible);
    QVERIFY(store.entry(pinned) == std::nullopt);
}

void TestDisplayImageStore::providerRequestsAreThreadSafeReads()
{
    auto store = std::make_shared<kiriview::DisplayImageStore>(4096);
    const QString id = store->insert(testEntry(QSize(16, 8)));
    kiriview::DisplayImageProvider provider(store);

    std::vector<std::future<QSize>> futures;
    for (int index = 0; index < 32; ++index) {
        futures.push_back(std::async(std::launch::async,
            [&provider, id]() { return provider.requestImage(id, nullptr, QSize(8, 8)).size(); }));
    }

    for (std::future<QSize>& future : futures) {
        QCOMPARE(future.get(), QSize(8, 4));
    }
}

QTEST_GUILESS_MAIN(TestDisplayImageStore)

#include "test_displayimagestore.moc"
