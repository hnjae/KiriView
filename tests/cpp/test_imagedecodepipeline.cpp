// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodepipeline.h"

#include <QByteArray>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
KiriView::ImageDecodePipelineStage unsupportedStage(const QString &name, QStringList *calls)
{
    return [name, calls](const KiriView::ImageDecodePipelineInput &) {
        calls->push_back(name);
        return std::optional<KiriView::DecodedImageResult>();
    };
}

KiriView::ImageDecodePipelineStage failureStage(
    const QString &name, const QString &errorString, QStringList *calls)
{
    return [name, errorString, calls](const KiriView::ImageDecodePipelineInput &) {
        calls->push_back(name);
        return std::optional<KiriView::DecodedImageResult>(
            KiriView::failedDecodedImageResult(errorString));
    };
}

KiriView::ImageDecodePipelineStage dataRecordingStage(
    QByteArray *originalData, QByteArray *compatibleData, quint64 *requestId)
{
    return
        [originalData, compatibleData, requestId](const KiriView::ImageDecodePipelineInput &input) {
            *originalData = input.originalData;
            *compatibleData = input.compatibleData;
            *requestId = input.request.id();
            return std::optional<KiriView::DecodedImageResult>(
                KiriView::failedDecodedImageResult(QStringLiteral("recorded")));
        };
}
}

class TestImageDecodePipeline : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void firstHandledStageStopsPipeline();
    void unsupportedStagesFallThrough();
    void stagesReceiveDecodeInputs();
};

void TestImageDecodePipeline::firstHandledStageStopsPipeline()
{
    QStringList calls;
    KiriView::ImageDecodePipeline pipeline({
        unsupportedStage(QStringLiteral("svg"), &calls),
        failureStage(QStringLiteral("apng"), QStringLiteral("APNG failed"), &calls),
        failureStage(QStringLiteral("fallback"), QStringLiteral("fallback ran"), &calls),
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
        unsupportedStage(QStringLiteral("svg"), &calls),
        unsupportedStage(QStringLiteral("apng"), &calls),
        failureStage(QStringLiteral("fallback"), QStringLiteral("fallback ran"), &calls),
    });

    const KiriView::DecodedImageResult result
        = pipeline.decode(QByteArrayLiteral("not an image"), KiriView::ImageDecodeRequest {});

    const auto *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("fallback ran"));
    QCOMPARE(calls,
        QStringList({ QStringLiteral("svg"), QStringLiteral("apng"), QStringLiteral("fallback") }));
}

void TestImageDecodePipeline::stagesReceiveDecodeInputs()
{
    QByteArray originalData;
    QByteArray compatibleData;
    quint64 requestId = 0;
    KiriView::ImageDecodePipeline pipeline({
        dataRecordingStage(&originalData, &compatibleData, &requestId),
    });
    const QByteArray inputData = QByteArrayLiteral("image bytes");
    const KiriView::ImageDecodeRequest request = KiriView::ImageDecodeRequest::fromUrl(
        42, QUrl::fromLocalFile(QStringLiteral("/tmp/image.png")));

    const KiriView::DecodedImageResult result = pipeline.decode(inputData, request);

    QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(originalData, inputData);
    QVERIFY(!compatibleData.isEmpty());
    QCOMPARE(requestId, quint64(42));
}

QTEST_GUILESS_MAIN(TestImageDecodePipeline)

#include "test_imagedecodepipeline.moc"
