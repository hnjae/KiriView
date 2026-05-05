// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagedecodejob.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::ManualImageDataLoader;

KiriView::DecodedImageResult decodeTestImageData(
    const QByteArray &data, const KiriView::ImageDecodeRequest &)
{
    if (data == QByteArrayLiteral("bad")) {
        return KiriView::DecodedImageFailure { QStringLiteral("decode failed") };
    }

    QImage image(1, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::successfulDecodedImageResult(
        KiriView::TestSupport::staticDecodedTestImage(image));
}

QUrl imageUrl(int index) { return QUrl(QStringLiteral("file:///images/%1.png").arg(index)); }
}

class TestImageDecodeJob : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cancelSuppressesPendingLoad();
    void staleLoadResultIsIgnored();
    void loadErrorsAreDeliveredForCurrentRequest();
    void decodeErrorsAreDeliveredAsResults();
    void decodeRequestIsPassedToDecoder();
};

void TestImageDecodeJob::cancelSuppressesPendingLoad()
{
    ManualImageDataLoader dataLoader;
    KiriView::ImageDecodeJob decodeJob(this, dataLoaderFor(dataLoader), decodeTestImageData);

    int decodedCount = 0;
    decodeJob.setDecodedCallback([&decodedCount](KiriView::ImageDecodeRequest,
                                     KiriView::DecodedImageResult) { ++decodedCount; });

    decodeJob.start(KiriView::ImageDecodeRequest { 1, imageUrl(1) });
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    decodeJob.cancel();

    QVERIFY(dataLoader.loads.front()->canceled);
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTest::qWait(50);
    QCOMPARE(decodedCount, 0);
}

void TestImageDecodeJob::staleLoadResultIsIgnored()
{
    ManualImageDataLoader dataLoader;
    KiriView::ImageDecodeJob decodeJob(this, dataLoaderFor(dataLoader), decodeTestImageData);

    std::vector<KiriView::ImageDecodeRequest> decodedRequests;
    decodeJob.setDecodedCallback(
        [&decodedRequests](KiriView::ImageDecodeRequest request, KiriView::DecodedImageResult) {
            decodedRequests.push_back(request);
        });

    decodeJob.start(KiriView::ImageDecodeRequest { 1, imageUrl(1) });
    decodeJob.start(KiriView::ImageDecodeRequest { 2, imageUrl(2) });
    QCOMPARE(dataLoader.loads.size(), std::size_t(2));
    QVERIFY(dataLoader.loads.front()->canceled);

    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_COMPARE(decodedRequests.size(), std::size_t(1));
    QCOMPARE(decodedRequests.front().id, quint64(2));
    QCOMPARE(decodedRequests.front().imageUrl, imageUrl(2));
}

void TestImageDecodeJob::loadErrorsAreDeliveredForCurrentRequest()
{
    ManualImageDataLoader dataLoader;
    KiriView::ImageDecodeJob decodeJob(this, dataLoaderFor(dataLoader), decodeTestImageData);

    std::vector<KiriView::ImageDecodeRequest> errorRequests;
    QString errorString;
    decodeJob.setLoadErrorCallback(
        [&errorRequests, &errorString](
            const KiriView::ImageDecodeRequest &request, const QString &error) {
            errorRequests.push_back(request);
            errorString = error;
        });

    decodeJob.start(KiriView::ImageDecodeRequest { 3, imageUrl(3) });
    dataLoader.loads.front()->errorCallback(QStringLiteral("missing"));

    QCOMPARE(errorRequests.size(), std::size_t(1));
    QCOMPARE(errorRequests.front().id, quint64(3));
    QCOMPARE(errorString, QStringLiteral("missing"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeErrorsAreDeliveredAsResults()
{
    ManualImageDataLoader dataLoader;
    KiriView::ImageDecodeJob decodeJob(this, dataLoaderFor(dataLoader), decodeTestImageData);

    std::optional<KiriView::DecodedImageResult> decodedResult;
    decodeJob.setDecodedCallback(
        [&decodedResult](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult result) {
            decodedResult = std::move(result);
        });

    decodeJob.start(KiriView::ImageDecodeRequest { 4, imageUrl(4) });
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("bad"));

    QTRY_VERIFY(decodedResult.has_value());
    const auto *failure = KiriView::decodedImageResultFailure(*decodedResult);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("decode failed"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

void TestImageDecodeJob::decodeRequestIsPassedToDecoder()
{
    ManualImageDataLoader dataLoader;
    std::optional<KiriView::ImageDecodeRequest> decoderRequest;
    KiriView::ImageDecodeJob decodeJob(this, dataLoaderFor(dataLoader),
        [&decoderRequest](const QByteArray &, const KiriView::ImageDecodeRequest &request) {
            decoderRequest = request;
            return KiriView::successfulDecodedImageResult(
                KiriView::TestSupport::staticDecodedTestImage());
        });

    decodeJob.setDecodedCallback(
        [](KiriView::ImageDecodeRequest, KiriView::DecodedImageResult) { });

    decodeJob.start(
        KiriView::ImageDecodeRequest { 5, imageUrl(5), KiriView::ArchiveDocumentLocation::none(),
            KiriView::ImageFirstDisplayDecodeContext { QSize(320, 200) } });
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("ok"));

    QTRY_VERIFY(decoderRequest.has_value());
    QCOMPARE(decoderRequest->id, quint64(5));
    QCOMPARE(decoderRequest->firstDisplay.physicalViewportSize, QSize(320, 200));
}

QTEST_GUILESS_MAIN(TestImageDecodeJob)

#include "test_imagedecodejob.moc"
