// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using DataSource = KiriView::ImageDecodePipelineDataSource;

KiriView::ImageDecodePipelineStage unsupportedStage(
    DataSource dataSource, const QString &name, QStringList *calls)
{
    return {
        dataSource,
        [name, calls](const KiriView::ImageDecodePipelineInput &) {
            calls->push_back(name);
            return KiriView::ImageDecodePipelineStageResult::unsupported();
        },
    };
}

KiriView::ImageDecodePipelineStage failureStage(
    DataSource dataSource, const QString &name, const QString &errorString, QStringList *calls)
{
    return {
        dataSource,
        [name, errorString, calls](const KiriView::ImageDecodePipelineInput &) {
            calls->push_back(name);
            return KiriView::ImageDecodePipelineStageResult::decoded(
                KiriView::failedDecodedImageResult(errorString));
        },
    };
}

KiriView::ImageDecodePipelineStage dataRecordingStage(DataSource dataSource,
    QByteArrayList *inputData, QList<quint64> *requestIds, bool handled = false)
{
    return {
        dataSource,
        [inputData, requestIds, handled](const KiriView::ImageDecodePipelineInput &input) {
            inputData->push_back(input.data);
            requestIds->push_back(input.request.id());
            if (!handled) {
                return KiriView::ImageDecodePipelineStageResult::unsupported();
            }
            return KiriView::ImageDecodePipelineStageResult::decoded(
                KiriView::failedDecodedImageResult(QStringLiteral("recorded")));
        },
    };
}
}

class TestImageDecodePipeline : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stageResultsDistinguishUnsupportedFromDecodedFailure();
    void firstHandledStageStopsPipeline();
    void unsupportedStagesFallThrough();
    void stagesReceiveSelectedDecodeInputs();
    void compatibleDataIsComputedOnlyWhenRequested();
    void compatibleDataIsSharedAcrossCompatibleStages();
};

void TestImageDecodePipeline::stageResultsDistinguishUnsupportedFromDecodedFailure()
{
    KiriView::ImageDecodePipelineStageResult unsupported
        = KiriView::ImageDecodePipelineStageResult::unsupported();
    QVERIFY(!unsupported.handled());
    QVERIFY(!std::move(unsupported).takeDecodedResult().has_value());

    KiriView::ImageDecodePipelineStageResult decoded
        = KiriView::ImageDecodePipelineStageResult::decoded(
            KiriView::failedDecodedImageResult(QStringLiteral("handled failure")));
    QVERIFY(decoded.handled());

    std::optional<KiriView::DecodedImageResult> result = std::move(decoded).takeDecodedResult();
    QVERIFY(result.has_value());
    const auto *failure = KiriView::decodedImageResultFailure(*result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("handled failure"));
}

void TestImageDecodePipeline::firstHandledStageStopsPipeline()
{
    QStringList calls;
    KiriView::ImageDecodePipeline pipeline({
        unsupportedStage(DataSource::Original, QStringLiteral("svg"), &calls),
        failureStage(
            DataSource::Original, QStringLiteral("apng"), QStringLiteral("APNG failed"), &calls),
        failureStage(DataSource::AvifCompatible, QStringLiteral("fallback"),
            QStringLiteral("fallback ran"), &calls),
    });

    const KiriView::DecodedImageResult result
        = pipeline.decode(QByteArrayLiteral("not an image"), KiriView::ImageDecodeRequest {});

    const auto *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("APNG failed"));
    QCOMPARE(calls, QStringList({ QStringLiteral("svg"), QStringLiteral("apng") }));
}

void TestImageDecodePipeline::unsupportedStagesFallThrough()
{
    QStringList calls;
    KiriView::ImageDecodePipeline pipeline({
        unsupportedStage(DataSource::Original, QStringLiteral("svg"), &calls),
        unsupportedStage(DataSource::Original, QStringLiteral("apng"), &calls),
        failureStage(DataSource::AvifCompatible, QStringLiteral("fallback"),
            QStringLiteral("fallback ran"), &calls),
    });

    const KiriView::DecodedImageResult result
        = pipeline.decode(QByteArrayLiteral("not an image"), KiriView::ImageDecodeRequest {});

    const auto *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("fallback ran"));
    QCOMPARE(calls,
        QStringList({ QStringLiteral("svg"), QStringLiteral("apng"), QStringLiteral("fallback") }));
}

void TestImageDecodePipeline::stagesReceiveSelectedDecodeInputs()
{
    QByteArrayList recordedInputData;
    QList<quint64> requestIds;
    const QByteArray originalData = QByteArrayLiteral("image bytes");
    const QByteArray compatibleData = QByteArrayLiteral("compatible image bytes");
    KiriView::ImageDecodePipeline pipeline(
        {
            dataRecordingStage(DataSource::Original, &recordedInputData, &requestIds),
            dataRecordingStage(DataSource::AvifCompatible, &recordedInputData, &requestIds, true),
        },
        [compatibleData](const QByteArray &) { return compatibleData; });
    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(
        42, QUrl::fromLocalFile(QStringLiteral("/tmp/image.png")));

    const KiriView::DecodedImageResult result = pipeline.decode(originalData, request);

    QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(recordedInputData, QByteArrayList({ originalData, compatibleData }));
    QCOMPARE(requestIds, QList<quint64>({ 42, 42 }));
}

void TestImageDecodePipeline::compatibleDataIsComputedOnlyWhenRequested()
{
    int compatibleTransformCount = 0;
    QStringList calls;
    KiriView::ImageDecodePipeline pipeline(
        {
            failureStage(
                DataSource::Original, QStringLiteral("original"), QStringLiteral("done"), &calls),
        },
        [&compatibleTransformCount](const QByteArray &data) {
            ++compatibleTransformCount;
            return data + QByteArrayLiteral("-compatible");
        });

    const KiriView::DecodedImageResult result
        = pipeline.decode(QByteArrayLiteral("image bytes"), KiriView::ImageDecodeRequest {});

    QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(calls, QStringList({ QStringLiteral("original") }));
    QCOMPARE(compatibleTransformCount, 0);
}

void TestImageDecodePipeline::compatibleDataIsSharedAcrossCompatibleStages()
{
    int compatibleTransformCount = 0;
    QByteArrayList inputData;
    QList<quint64> requestIds;
    KiriView::ImageDecodePipeline pipeline(
        {
            dataRecordingStage(DataSource::AvifCompatible, &inputData, &requestIds),
            dataRecordingStage(DataSource::AvifCompatible, &inputData, &requestIds, true),
        },
        [&compatibleTransformCount](const QByteArray &data) {
            ++compatibleTransformCount;
            return data + QByteArrayLiteral("-compatible");
        });

    const KiriView::DecodedImageResult result
        = pipeline.decode(QByteArrayLiteral("image bytes"), KiriView::ImageDecodeRequest {});

    QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(inputData,
        QByteArrayList({ QByteArrayLiteral("image bytes-compatible"),
            QByteArrayLiteral("image bytes-compatible") }));
    QCOMPARE(compatibleTransformCount, 1);
}

QTEST_GUILESS_MAIN(TestImageDecodePipeline)

#include "test_imagedecodepipeline.moc"
