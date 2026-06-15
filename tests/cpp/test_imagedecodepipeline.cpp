// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodepipeline.h"

#include "localization/imageerrortext.h"

#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <utility>

namespace {
kiriview::ImageInputClassification classification(kiriview::ImageInputKind kind,
    kiriview::QtRasterFormat qtFormat = kiriview::QtRasterFormat::None,
    kiriview::ImageDecodeDataSource dataSource = kiriview::ImageDecodeDataSource::Original)
{
    return kiriview::ImageInputClassification {
        kind,
        qtFormat,
        dataSource,
    };
}

kiriview::ImageDecodeRouterHandler recordingHandler(const QString &name, QStringList *calls,
    QByteArrayList *inputData = nullptr, QList<kiriview::QtRasterFormat> *qtFormats = nullptr,
    QString errorString = {})
{
    return [name, calls, inputData, qtFormats, errorString](
               const kiriview::ImageDecodeRouterInput &input) {
        calls->push_back(name);
        if (inputData != nullptr) {
            inputData->push_back(input.data);
        }
        if (qtFormats != nullptr) {
            qtFormats->push_back(input.qtRasterFormat);
        }
        return kiriview::failedDecodedImageResult(
            errorString.isEmpty() ? name + QStringLiteral(" failed") : errorString);
    };
}

kiriview::ImageDecodeRouterHandlers recordingHandlers(QStringList *calls,
    QByteArrayList *inputData = nullptr, QList<kiriview::QtRasterFormat> *qtFormats = nullptr)
{
    return kiriview::ImageDecodeRouterHandlers {
        recordingHandler(QStringLiteral("svg"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("apng"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("heif"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("raw"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("qt"), calls, inputData, qtFormats),
    };
}

QByteArray invalidJxlContainerData()
{
    QByteArray data;
    data.append(char(0x00));
    data.append(char(0x00));
    data.append(char(0x00));
    data.append(char(0x0c));
    data.append("JXL ", 4);
    data.append(char(0x0d));
    data.append(char(0x0a));
    data.append(char(0x87));
    data.append(char(0x0a));
    return data;
}
}

class TestImageDecodePipeline : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void routePlanKeepsClassificationSeparateFromDecoderExecution();
    void runtimeExecutesRoutePlansWithoutClassifier();
    void routerCallsExactlyOneDecoderForClassifiedInputs();
    void selectedDecoderFailureDoesNotFallback();
    void compatibleDataIsComputedOnlyWhenClassificationRequestsIt();
    void qtRasterClassificationCarriesExplicitFormat();
    void defaultSvgDecodeUsesFirstDisplayContext();
    void defaultSvgOpenFailurePreservesAdapterDiagnostics();
    void defaultApngOpenFailurePreservesAdapterDiagnostics();
    void defaultJxlAnimationOpenFailurePreservesAdapterDiagnostics();
    void unknownClassificationFailsWithoutDecoder();
};

void TestImageDecodePipeline::routePlanKeepsClassificationSeparateFromDecoderExecution()
{
    struct Case {
        kiriview::ImageInputClassification classification;
        kiriview::ImageDecodeHandlerKind expectedHandler;
        kiriview::ImageDecodeDataSource expectedDataSource;
        kiriview::QtRasterFormat expectedFormat;
        bool expectedShouldDecode = true;
    };
    const QList<Case> cases {
        { classification(kiriview::ImageInputKind::Svg), kiriview::ImageDecodeHandlerKind::Svg,
            kiriview::ImageDecodeDataSource::Original, kiriview::QtRasterFormat::None },
        { classification(kiriview::ImageInputKind::Apng), kiriview::ImageDecodeHandlerKind::Apng,
            kiriview::ImageDecodeDataSource::Original, kiriview::QtRasterFormat::None },
        { classification(kiriview::ImageInputKind::HeifFamily, kiriview::QtRasterFormat::None,
              kiriview::ImageDecodeDataSource::AvifCompatible),
            kiriview::ImageDecodeHandlerKind::HeifFamily,
            kiriview::ImageDecodeDataSource::AvifCompatible, kiriview::QtRasterFormat::None },
        { classification(kiriview::ImageInputKind::Raw), kiriview::ImageDecodeHandlerKind::Raw,
            kiriview::ImageDecodeDataSource::Original, kiriview::QtRasterFormat::None },
        { classification(kiriview::ImageInputKind::QtRaster, kiriview::QtRasterFormat::Jxl),
            kiriview::ImageDecodeHandlerKind::QtRaster, kiriview::ImageDecodeDataSource::Original,
            kiriview::QtRasterFormat::Jxl },
        { classification(kiriview::ImageInputKind::Unknown, kiriview::QtRasterFormat::None,
              kiriview::ImageDecodeDataSource::AvifCompatible),
            kiriview::ImageDecodeHandlerKind::None, kiriview::ImageDecodeDataSource::Original,
            kiriview::QtRasterFormat::None, false },
    };

    for (const Case &testCase : cases) {
        const kiriview::ImageDecodeRoute route
            = kiriview::imageDecodeRouteForClassification(testCase.classification);
        QCOMPARE(route.handlerKind, testCase.expectedHandler);
        QCOMPARE(route.dataSource, testCase.expectedDataSource);
        QCOMPARE(route.qtRasterFormat, testCase.expectedFormat);
        QCOMPARE(route.shouldDecode(), testCase.expectedShouldDecode);
    }
}

void TestImageDecodePipeline::runtimeExecutesRoutePlansWithoutClassifier()
{
    const QByteArray originalData = QByteArrayLiteral("original image bytes");
    const QByteArray compatibleData = QByteArrayLiteral("compatible image bytes");

    int compatibleTransformCount = 0;
    QStringList calls;
    QByteArrayList inputData;
    QList<kiriview::QtRasterFormat> qtFormats;
    kiriview::ImageDecodeRouterRuntime runtime(recordingHandlers(&calls, &inputData, &qtFormats),
        [&compatibleTransformCount, compatibleData](const QByteArray &) {
            ++compatibleTransformCount;
            return compatibleData;
        });

    const kiriview::ImageDecodeRoute route {
        kiriview::ImageDecodeHandlerKind::QtRaster,
        kiriview::ImageDecodeDataSource::AvifCompatible,
        kiriview::QtRasterFormat::Jxl,
    };

    runtime.execute(route, originalData, kiriview::ImageDecodeRequest {});

    QCOMPARE(compatibleTransformCount, 1);
    QCOMPARE(calls, QStringList({ QStringLiteral("qt") }));
    QCOMPARE(inputData, QByteArrayList({ compatibleData }));
    QCOMPARE(qtFormats, QList<kiriview::QtRasterFormat>({ kiriview::QtRasterFormat::Jxl }));
}

void TestImageDecodePipeline::routerCallsExactlyOneDecoderForClassifiedInputs()
{
    struct Case {
        kiriview::ImageInputClassification classification;
        QString expectedCall;
    };
    const QList<Case> cases {
        { classification(kiriview::ImageInputKind::Svg), QStringLiteral("svg") },
        { classification(kiriview::ImageInputKind::Apng), QStringLiteral("apng") },
        { classification(kiriview::ImageInputKind::HeifFamily), QStringLiteral("heif") },
        { classification(kiriview::ImageInputKind::Raw), QStringLiteral("raw") },
        { classification(kiriview::ImageInputKind::QtRaster, kiriview::QtRasterFormat::Png),
            QStringLiteral("qt") },
    };

    for (const Case &testCase : cases) {
        QStringList calls;
        kiriview::ImageDecodeRouter router(recordingHandlers(&calls),
            [testCase](const QByteArray &, const QString &) { return testCase.classification; });

        const kiriview::DecodedImageResult result
            = router.decode(QByteArrayLiteral("image bytes"), kiriview::ImageDecodeRequest {});

        QVERIFY(kiriview::decodedImageResultFailure(result) != nullptr);
        QCOMPARE(calls, QStringList({ testCase.expectedCall }));
    }
}

void TestImageDecodePipeline::selectedDecoderFailureDoesNotFallback()
{
    QStringList calls;
    kiriview::ImageDecodeRouterHandlers handlers = recordingHandlers(&calls);
    handlers.raw = recordingHandler(
        QStringLiteral("raw"), &calls, nullptr, nullptr, QStringLiteral("raw failed"));
    handlers.qtRaster = recordingHandler(
        QStringLiteral("qt"), &calls, nullptr, nullptr, QStringLiteral("qt fallback ran"));
    kiriview::ImageDecodeRouter router(
        std::move(handlers), [](const QByteArray &, const QString &) {
            return classification(kiriview::ImageInputKind::Raw);
        });

    const kiriview::DecodedImageResult result
        = router.decode(QByteArrayLiteral("not raw"), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("raw failed"));
    QCOMPARE(failure->route, kiriview::DecodedImageFailureRoute::Raw);
    QCOMPARE(calls, QStringList({ QStringLiteral("raw") }));
}

void TestImageDecodePipeline::compatibleDataIsComputedOnlyWhenClassificationRequestsIt()
{
    const QByteArray originalData = QByteArrayLiteral("original image bytes");
    const QByteArray compatibleData = QByteArrayLiteral("compatible image bytes");

    int qtCompatibleTransformCount = 0;
    QStringList qtCalls;
    QByteArrayList qtInputData;
    kiriview::ImageDecodeRouter qtRouter(
        recordingHandlers(&qtCalls, &qtInputData),
        [](const QByteArray &, const QString &) {
            return classification(
                kiriview::ImageInputKind::QtRaster, kiriview::QtRasterFormat::Png);
        },
        [&qtCompatibleTransformCount, compatibleData](const QByteArray &) {
            ++qtCompatibleTransformCount;
            return compatibleData;
        });

    qtRouter.decode(originalData, kiriview::ImageDecodeRequest {});

    QCOMPARE(qtCompatibleTransformCount, 0);
    QCOMPARE(qtCalls, QStringList({ QStringLiteral("qt") }));
    QCOMPARE(qtInputData, QByteArrayList({ originalData }));

    int heifCompatibleTransformCount = 0;
    QStringList heifCalls;
    QByteArrayList heifInputData;
    kiriview::ImageDecodeRouter heifRouter(
        recordingHandlers(&heifCalls, &heifInputData),
        [](const QByteArray &, const QString &) {
            return classification(kiriview::ImageInputKind::HeifFamily,
                kiriview::QtRasterFormat::None, kiriview::ImageDecodeDataSource::AvifCompatible);
        },
        [&heifCompatibleTransformCount, compatibleData](const QByteArray &) {
            ++heifCompatibleTransformCount;
            return compatibleData;
        });

    heifRouter.decode(originalData, kiriview::ImageDecodeRequest {});

    QCOMPARE(heifCompatibleTransformCount, 1);
    QCOMPARE(heifCalls, QStringList({ QStringLiteral("heif") }));
    QCOMPARE(heifInputData, QByteArrayList({ compatibleData }));
}

void TestImageDecodePipeline::qtRasterClassificationCarriesExplicitFormat()
{
    const QList<kiriview::QtRasterFormat> formats {
        kiriview::QtRasterFormat::Png,
        kiriview::QtRasterFormat::Jpeg,
        kiriview::QtRasterFormat::Gif,
        kiriview::QtRasterFormat::Webp,
        kiriview::QtRasterFormat::Bmp,
        kiriview::QtRasterFormat::Tiff,
        kiriview::QtRasterFormat::Jxl,
        kiriview::QtRasterFormat::Jp2,
    };

    for (kiriview::QtRasterFormat format : formats) {
        QStringList calls;
        QList<kiriview::QtRasterFormat> qtFormats;
        kiriview::ImageDecodeRouter router(recordingHandlers(&calls, nullptr, &qtFormats),
            [format](const QByteArray &, const QString &) {
                return classification(kiriview::ImageInputKind::QtRaster, format);
            });

        router.decode(QByteArrayLiteral("raster bytes"), kiriview::ImageDecodeRequest {});

        QCOMPARE(calls, QStringList({ QStringLiteral("qt") }));
        QCOMPARE(qtFormats, QList<kiriview::QtRasterFormat>({ format }));
    }
}

void TestImageDecodePipeline::defaultSvgDecodeUsesFirstDisplayContext()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");
    const kiriview::DecodedImageResult result = kiriview::decodeImageDataWithDefaultRouter(data,
        kiriview::ImageDecodeRequest::fromUrl(7, QUrl(QStringLiteral("file:///tmp/vector.svg")),
            kiriview::ImageFirstDisplayDecodeContext { QSize(200, 200) }));

    const auto *image = kiriview::decodedImageResultImageAs<kiriview::StaticDecodedImage>(result);
    QVERIFY(image != nullptr);
    QCOMPARE(image->displayImage.image.size(), QSize(200, 100));
    QCOMPARE(image->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(image->displayImage.displayPixelsPerSourcePixel, 2.5);
}

void TestImageDecodePipeline::defaultSvgOpenFailurePreservesAdapterDiagnostics()
{
    const kiriview::ImageDecodeRoute route {
        kiriview::ImageDecodeHandlerKind::Svg,
        kiriview::ImageDecodeDataSource::Original,
        kiriview::QtRasterFormat::None,
    };
    const kiriview::ImageDecodeRouterRuntime runtime({});

    const kiriview::DecodedImageResult result
        = runtime.execute(route, QByteArrayLiteral("<svg"), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->route, kiriview::DecodedImageFailureRoute::Svg);
    QVERIFY(failure->operation != kiriview::DecodedImageFailureOperation::Unknown);
    QVERIFY(!failure->diagnosticDetail.isEmpty());
    QVERIFY(failure->diagnosticDetail != failure->errorString);
    QVERIFY(kiriview::decodedImageResultImage(result) == nullptr);
}

void TestImageDecodePipeline::defaultApngOpenFailurePreservesAdapterDiagnostics()
{
    const kiriview::ImageDecodeRoute route {
        kiriview::ImageDecodeHandlerKind::Apng,
        kiriview::ImageDecodeDataSource::Original,
        kiriview::QtRasterFormat::None,
    };
    const kiriview::ImageDecodeRouterRuntime runtime({});

    const kiriview::DecodedImageResult result
        = runtime.execute(route, QByteArrayLiteral("not an apng"), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->route, kiriview::DecodedImageFailureRoute::Apng);
    QVERIFY(failure->operation != kiriview::DecodedImageFailureOperation::Unknown);
    QVERIFY(!failure->diagnosticDetail.isEmpty());
    QVERIFY(failure->diagnosticDetail != failure->errorString);
    QVERIFY(kiriview::decodedImageResultImage(result) == nullptr);
}

void TestImageDecodePipeline::defaultJxlAnimationOpenFailurePreservesAdapterDiagnostics()
{
    const kiriview::ImageDecodeRoute route {
        kiriview::ImageDecodeHandlerKind::QtRaster,
        kiriview::ImageDecodeDataSource::Original,
        kiriview::QtRasterFormat::Jxl,
    };
    const kiriview::ImageDecodeRouterRuntime runtime({});

    const kiriview::DecodedImageResult result
        = runtime.execute(route, invalidJxlContainerData(), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->route, kiriview::DecodedImageFailureRoute::QtRaster);
    QVERIFY(failure->operation != kiriview::DecodedImageFailureOperation::Unknown);
    QVERIFY(!failure->diagnosticDetail.isEmpty());
    QVERIFY(failure->diagnosticDetail != failure->errorString);
    QVERIFY(kiriview::decodedImageResultImage(result) == nullptr);
}

void TestImageDecodePipeline::unknownClassificationFailsWithoutDecoder()
{
    int compatibleTransformCount = 0;
    QStringList calls;
    kiriview::ImageDecodeRouter router(
        recordingHandlers(&calls),
        [](const QByteArray &, const QString &) {
            return classification(kiriview::ImageInputKind::Unknown, kiriview::QtRasterFormat::None,
                kiriview::ImageDecodeDataSource::AvifCompatible);
        },
        [&compatibleTransformCount](const QByteArray &data) {
            ++compatibleTransformCount;
            return data + QByteArrayLiteral("-compatible");
        });

    const kiriview::DecodedImageResult result
        = router.decode(QByteArrayLiteral("unknown bytes"), kiriview::ImageDecodeRequest {});

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(
        failure->errorString, kiriview::imageErrorText(kiriview::ImageErrorTextId::ReadImageData));
    QVERIFY(calls.isEmpty());
    QCOMPARE(compatibleTransformCount, 0);
}

QTEST_GUILESS_MAIN(TestImageDecodePipeline)

#include "test_imagedecodepipeline.moc"
