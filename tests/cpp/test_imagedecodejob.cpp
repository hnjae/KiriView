// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagedecodejob.h"

#include <QByteArray>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::testImageDecodeFailureString;

KiriView::ImageDecodeJob::Callbacks decodeJobCallbacks(
    KiriView::ImageDecodeJob::DecodedCallback decoded = {},
    KiriView::ImageDecodeJob::LoadErrorCallback loadError = {})
{
    return KiriView::ImageDecodeJob::Callbacks { std::move(decoded), std::move(loadError) };
}
}

class TestImageDecodeJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancelSuppressesPendingLoad();
    void staleLoadResultIsIgnored();
    void restartedSameRequestIgnoresStaleLoadResult();
    void loadErrorsAreDeliveredForCurrentRequest();
    void decodeErrorsAreDeliveredAsResults();
    void decodeRequestIsPassedToDecoder();
};

void TestImageDecodeJob::cancelSuppressesPendingLoad()
{
    ManualImageDataLoader dataLoader;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    decodeJob.cancel();

    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTest::qWait(50);
    QCOMPARE(decodedCount, 0);
}

void TestImageDecodeJob::staleLoadResultIsIgnored()
{
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDecodeRequest> decodedRequests;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedRequests](KiriView::ImageDecodeRequest request, KiriView::DecodedImageResult) {
                decodedRequests.push_back(request);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(1, indexedImageUrl(1)));
    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(2, indexedImageUrl(2)));
    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("ok"));
    dataLoader.finishBackLoad(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedRequests.size(), std::size_t(1));
    QCOMPARE(decodedRequests.front().id(), quint64(2));
    QCOMPARE(decodedRequests.front().imageUrl(), indexedImageUrl(2));
}

void TestImageDecodeJob::restartedSameRequestIgnoresStaleLoadResult()
{
    ManualImageDataLoader dataLoader;
    QByteArray decodedData;
    int decodedCount = 0;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decodedData](const QByteArray &data, const KiriView::ImageDecodeRequest &) {
                decodedData = data;
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([&decodedCount](KiriView::ImageDecodeRequest,
                               KiriView::DecodedImageResult) { ++decodedCount; }));
    const KiriView::ImageDecodeRequest request
        = KiriView::ImageDecodeRequest::fromUrl(7, indexedImageUrl(7));

    decodeJob.start(request);
    decodeJob.start(request);
    QCOMPARE(dataLoader.loadCount(), std::size_t(2));
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("old"));
    QTest::qWait(50);
    QCOMPARE(decodedCount, 0);

    dataLoader.finishBackLoad(QByteArrayLiteral("new"));

    QTRY_COMPARE(decodedCount, 1);
    QCOMPARE(decodedData, QByteArrayLiteral("new"));
}

void TestImageDecodeJob::loadErrorsAreDeliveredForCurrentRequest()
{
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDecodeRequest> errorRequests;
    QString errorString;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks({},
            [&errorRequests, &errorString](
                const KiriView::ImageDecodeRequest &request, const QString &error) {
                errorRequests.push_back(request);
                errorString = error;
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(3, indexedImageUrl(3)));
    dataLoader.failFrontLoad(QStringLiteral("missing"));

    QCOMPARE(errorRequests.size(), std::size_t(1));
    QCOMPARE(errorRequests.front().id(), quint64(3));
    QCOMPARE(errorString, QStringLiteral("missing"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeErrorsAreDeliveredAsResults()
{
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::DecodedImageResult> decodedResult;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoderRejectingBadData()),
        decodeJobCallbacks(
            [&decodedResult](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult result) {
                decodedResult = std::move(result);
            }));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(4, indexedImageUrl(4)));
    dataLoader.finishFrontLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(decodedResult.has_value());
    const auto *failure = KiriView::decodedImageResultFailure(*decodedResult);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, testImageDecodeFailureString());
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeRequestIsPassedToDecoder()
{
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageDecodeRequest> decoderRequest;
    KiriView::ImageDecodeJob decodeJob(this,
        imageDecodeDependenciesFor(dataLoader,
            [&decoderRequest](const QByteArray &, const KiriView::ImageDecodeRequest &request) {
                decoderRequest = request;
                return KiriView::successfulDecodedImageResult(
                    KiriView::TestSupport::staticDecodedTestImage());
            }),
        decodeJobCallbacks([](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult) {}));

    decodeJob.start(KiriView::ImageDecodeRequest::fromUrl(
        5, indexedImageUrl(5), KiriView::ImageFirstDisplayDecodeContext { QSize(320, 200) }));
    dataLoader.finishFrontLoad(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decoderRequest.has_value());
    QCOMPARE(decoderRequest->id(), quint64(5));
    QCOMPARE(decoderRequest->firstDisplay().physicalViewportSize, QSize(320, 200));
}

QTEST_GUILESS_MAIN(TestImageDecodeJob)

#include "test_imagedecodejob.moc"
