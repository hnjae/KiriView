// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejob.h"
#include "image_test_support.h"
#include "location/imageurl.h"
#include "predecode/predecodeactivedecodestore.h"

#include <QCoreApplication>
#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QTest>
#include <optional>
#include <vector>

namespace {
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticImageDataDecoder;

kiriview::ImageDecodeRequest decodeRequest(quint64 id, const QUrl &url)
{
    return kiriview::ImageDecodeRequest::fromUrl(id, url);
}

void sendDeferredDeletes() { QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
}

class TestPredecodeActiveDecodeStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void addRejectsDuplicateOrInvalidRequests();
    void finishReturnsMatchingRequestAndDeletesJob();
    void finishRejectsStaleGenerationAndUrl();
    void finishRemovesOnlyMatchingEntry();
    void cancelCancelsDecodeJobsAndClearsStore();
};

void TestPredecodeActiveDecodeStore::addRejectsDuplicateOrInvalidRequests()
{
    kiriview::PredecodeActiveDecodeStore store;
    auto *firstJob = new kiriview::ImageDecodeJob(this);
    auto *duplicateJob = new kiriview::ImageDecodeJob(this);
    const QUrl url = indexedImageUrl(1);

    QVERIFY(store.add(decodeRequest(7, url), firstJob));
    QVERIFY(!store.add(decodeRequest(8, url), duplicateJob));
    QVERIFY(!store.add(kiriview::ImageDecodeRequest {}, firstJob));
    QVERIFY(!store.add(decodeRequest(9, indexedImageUrl(2)), nullptr));
    QCOMPARE(store.size(), std::size_t(1));

    duplicateJob->deleteLater();
    store.cancel();
    sendDeferredDeletes();
}

void TestPredecodeActiveDecodeStore::finishReturnsMatchingRequestAndDeletesJob()
{
    kiriview::PredecodeActiveDecodeStore store;
    auto *job = new kiriview::ImageDecodeJob(this);
    QPointer<kiriview::ImageDecodeJob> jobGuard(job);
    const QUrl url = indexedImageUrl(1);

    QVERIFY(store.add(decodeRequest(7, url), job));

    QCOMPARE(store.size(), std::size_t(1));
    QVERIFY(store.containsUrl(url));
    const kiriview::PredecodeActiveLoads activeLoads = store.activeLoads();
    QCOMPARE(activeLoads.size(), std::size_t(1));
    QVERIFY(activeLoads.contains(url));
    QVERIFY(activeLoads.contains(kiriview::normalizedImageUrl(url)));

    const std::optional<kiriview::ImageDecodeRequest> finished
        = store.finish(decodeRequest(7, url));

    QVERIFY(finished.has_value());
    QCOMPARE(finished->id(), quint64(7));
    QCOMPARE(finished->imageUrl(), url);
    QCOMPARE(store.size(), std::size_t(0));

    sendDeferredDeletes();
    QVERIFY(jobGuard.isNull());
}

void TestPredecodeActiveDecodeStore::finishRejectsStaleGenerationAndUrl()
{
    kiriview::PredecodeActiveDecodeStore store;
    auto *job = new kiriview::ImageDecodeJob(this);
    const QUrl url = indexedImageUrl(1);
    const QUrl otherUrl = indexedImageUrl(2);

    QVERIFY(store.add(decodeRequest(7, url), job));

    QVERIFY(!store.finish(decodeRequest(8, url)).has_value());
    QVERIFY(!store.finish(decodeRequest(7, otherUrl)).has_value());
    QCOMPARE(store.size(), std::size_t(1));

    QVERIFY(store.finish(decodeRequest(7, url)).has_value());
    sendDeferredDeletes();
}

void TestPredecodeActiveDecodeStore::finishRemovesOnlyMatchingEntry()
{
    kiriview::PredecodeActiveDecodeStore store;
    auto *firstJob = new kiriview::ImageDecodeJob(this);
    auto *secondJob = new kiriview::ImageDecodeJob(this);
    QPointer<kiriview::ImageDecodeJob> firstJobGuard(firstJob);
    QPointer<kiriview::ImageDecodeJob> secondJobGuard(secondJob);
    const QUrl firstUrl = indexedImageUrl(1);
    const QUrl secondUrl = indexedImageUrl(2);

    QVERIFY(store.add(decodeRequest(7, firstUrl), firstJob));
    QVERIFY(store.add(decodeRequest(7, secondUrl), secondJob));

    QVERIFY(store.finish(decodeRequest(7, firstUrl)).has_value());

    QCOMPARE(store.size(), std::size_t(1));
    QVERIFY(!store.containsUrl(firstUrl));
    QVERIFY(store.containsUrl(secondUrl));
    sendDeferredDeletes();
    QVERIFY(firstJobGuard.isNull());
    QVERIFY(!secondJobGuard.isNull());

    store.cancel();
    sendDeferredDeletes();
    QVERIFY(secondJobGuard.isNull());
}

void TestPredecodeActiveDecodeStore::cancelCancelsDecodeJobsAndClearsStore()
{
    ManualImageDataLoader dataLoader;
    kiriview::ImageDecodeDependencies dependencies
        = imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder());
    kiriview::PredecodeActiveDecodeStore store;
    auto *job = new kiriview::ImageDecodeJob(this, dependencies);
    QPointer<kiriview::ImageDecodeJob> jobGuard(job);
    const kiriview::ImageDecodeRequest request = decodeRequest(7, indexedImageUrl(1));

    job->start(request);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QVERIFY(store.add(request, job));

    store.cancel();

    QCOMPARE(store.size(), std::size_t(0));
    QVERIFY(dataLoader.frontLoad().canceled);
    sendDeferredDeletes();
    QVERIFY(jobGuard.isNull());
}

QTEST_GUILESS_MAIN(TestPredecodeActiveDecodeStore)

#include "test_predecodeactivedecodestore.moc"
