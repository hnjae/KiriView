// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejob.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <utility>
#include <vector>

namespace {
struct ManualLoad {
    QObject *object = nullptr;
    QUrl url;
    KiriView::ImageDecodeJob::DataCallback dataCallback;
    KiriView::ImageDecodeJob::ErrorCallback errorCallback;
    bool canceled = false;
};

using DecodedResultPtr = std::shared_ptr<KiriView::DecodedImageResult>;

class ManualImageDataLoader
{
public:
    KiriView::ImageIoJob start(QObject *receiver, QUrl url,
        KiriView::ImageDecodeJob::DataCallback callback,
        KiriView::ImageDecodeJob::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualLoad>();
        load->object = new QObject(receiver);
        load->url = std::move(url);
        load->dataCallback = std::move(callback);
        load->errorCallback = std::move(errorCallback);
        loads.push_back(load);

        return KiriView::ImageIoJob(load->object, [load](QObject *object) {
            load->canceled = true;
            if (object != nullptr) {
                object->deleteLater();
            }
        });
    }

    std::vector<std::shared_ptr<ManualLoad>> loads;
};

KiriView::DecodedImageResult decodeTestImageData(const QByteArray &data)
{
    KiriView::DecodedImageResult result;
    if (data == QByteArrayLiteral("bad")) {
        result.errorString = QStringLiteral("decode failed");
        return result;
    }

    result.success = true;
    result.image = QImage(1, 1, QImage::Format_RGBA8888_Premultiplied);
    result.image.fill(Qt::transparent);
    return result;
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
};

void TestImageDecodeJob::cancelSuppressesPendingLoad()
{
    ManualImageDataLoader dataLoader;
    KiriView::ImageDecodeJob decodeJob(
        this,
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);

    int decodedCount = 0;
    decodeJob.setDecodedCallback(
        [&decodedCount](KiriView::ImageDecodeRequest,
            std::shared_ptr<KiriView::DecodedImageResult>) { ++decodedCount; });

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
    KiriView::ImageDecodeJob decodeJob(
        this,
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);

    std::vector<KiriView::ImageDecodeRequest> decodedRequests;
    decodeJob.setDecodedCallback(
        [&decodedRequests](KiriView::ImageDecodeRequest request,
            std::shared_ptr<KiriView::DecodedImageResult>) { decodedRequests.push_back(request); });

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
    KiriView::ImageDecodeJob decodeJob(
        this,
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);

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
    KiriView::ImageDecodeJob decodeJob(
        this,
        [&dataLoader](QObject *receiver, QUrl url, KiriView::ImageDecodeJob::DataCallback callback,
            KiriView::ImageDecodeJob::ErrorCallback errorCallback) {
            return dataLoader.start(
                receiver, std::move(url), std::move(callback), std::move(errorCallback));
        },
        decodeTestImageData);

    DecodedResultPtr decodedResult;
    decodeJob.setDecodedCallback(
        [&decodedResult](KiriView::ImageDecodeRequest, DecodedResultPtr result) {
            decodedResult = std::move(result);
        });

    decodeJob.start(KiriView::ImageDecodeRequest { 4, imageUrl(4) });
    dataLoader.loads.front()->dataCallback(QByteArrayLiteral("bad"));

    QTRY_VERIFY(decodedResult != nullptr);
    QVERIFY(!decodedResult->success);
    QCOMPARE(decodedResult->errorString, QStringLiteral("decode failed"));
    QVERIFY(!decodeJob.hasActiveRequest());
}

QTEST_GUILESS_MAIN(TestImageDecodeJob)

#include "test_imagedecodejob.moc"
