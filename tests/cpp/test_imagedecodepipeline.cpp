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
KiriView::ImageInputClassification classification(KiriView::ImageInputKind kind,
    KiriView::QtRasterFormat qtFormat = KiriView::QtRasterFormat::None,
    KiriView::ImageDecodeDataSource dataSource = KiriView::ImageDecodeDataSource::Original)
{
    return KiriView::ImageInputClassification {
        kind,
        qtFormat,
        dataSource,
    };
}

KiriView::ImageDecodeRouterHandler recordingHandler(const QString &name, QStringList *calls,
    QByteArrayList *inputData = nullptr, QList<KiriView::QtRasterFormat> *qtFormats = nullptr,
    QString errorString = {})
{
    return [name, calls, inputData, qtFormats, errorString](
               const KiriView::ImageDecodeRouterInput &input) {
        calls->push_back(name);
        if (inputData != nullptr) {
            inputData->push_back(input.data);
        }
        if (qtFormats != nullptr) {
            qtFormats->push_back(input.qtRasterFormat);
        }
        return KiriView::failedDecodedImageResult(
            errorString.isEmpty() ? name + QStringLiteral(" failed") : errorString);
    };
}

KiriView::ImageDecodeRouterHandlers recordingHandlers(QStringList *calls,
    QByteArrayList *inputData = nullptr, QList<KiriView::QtRasterFormat> *qtFormats = nullptr)
{
    return KiriView::ImageDecodeRouterHandlers {
        recordingHandler(QStringLiteral("svg"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("apng"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("heif"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("raw"), calls, inputData, qtFormats),
        recordingHandler(QStringLiteral("qt"), calls, inputData, qtFormats),
    };
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
    void unknownClassificationFailsWithoutDecoder();
};

void TestImageDecodePipeline::routePlanKeepsClassificationSeparateFromDecoderExecution()
{
    struct Case {
        KiriView::ImageInputClassification classification;
        KiriView::ImageDecodeHandlerKind expectedHandler;
        KiriView::ImageDecodeDataSource expectedDataSource;
        KiriView::QtRasterFormat expectedFormat;
        bool expectedShouldDecode = true;
    };
    const QList<Case> cases {
        { classification(KiriView::ImageInputKind::Svg), KiriView::ImageDecodeHandlerKind::Svg,
            KiriView::ImageDecodeDataSource::Original, KiriView::QtRasterFormat::None },
        { classification(KiriView::ImageInputKind::Apng), KiriView::ImageDecodeHandlerKind::Apng,
            KiriView::ImageDecodeDataSource::Original, KiriView::QtRasterFormat::None },
        { classification(KiriView::ImageInputKind::HeifFamily, KiriView::QtRasterFormat::None,
              KiriView::ImageDecodeDataSource::AvifCompatible),
            KiriView::ImageDecodeHandlerKind::HeifFamily,
            KiriView::ImageDecodeDataSource::AvifCompatible, KiriView::QtRasterFormat::None },
        { classification(KiriView::ImageInputKind::Raw), KiriView::ImageDecodeHandlerKind::Raw,
            KiriView::ImageDecodeDataSource::Original, KiriView::QtRasterFormat::None },
        { classification(KiriView::ImageInputKind::QtRaster, KiriView::QtRasterFormat::Jxl),
            KiriView::ImageDecodeHandlerKind::QtRaster, KiriView::ImageDecodeDataSource::Original,
            KiriView::QtRasterFormat::Jxl },
        { classification(KiriView::ImageInputKind::Unknown, KiriView::QtRasterFormat::None,
              KiriView::ImageDecodeDataSource::AvifCompatible),
            KiriView::ImageDecodeHandlerKind::None, KiriView::ImageDecodeDataSource::Original,
            KiriView::QtRasterFormat::None, false },
    };

    for (const Case &testCase : cases) {
        const KiriView::ImageDecodeRoute route
            = KiriView::imageDecodeRouteForClassification(testCase.classification);
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
    QList<KiriView::QtRasterFormat> qtFormats;
    KiriView::ImageDecodeRouterRuntime runtime(recordingHandlers(&calls, &inputData, &qtFormats),
        [&compatibleTransformCount, compatibleData](const QByteArray &) {
            ++compatibleTransformCount;
            return compatibleData;
        });

    const KiriView::ImageDecodeRoute route {
        KiriView::ImageDecodeHandlerKind::QtRaster,
        KiriView::ImageDecodeDataSource::AvifCompatible,
        KiriView::QtRasterFormat::Jxl,
    };

    runtime.execute(route, originalData, KiriView::ImageDecodeRequest {});

    QCOMPARE(compatibleTransformCount, 1);
    QCOMPARE(calls, QStringList({ QStringLiteral("qt") }));
    QCOMPARE(inputData, QByteArrayList({ compatibleData }));
    QCOMPARE(qtFormats, QList<KiriView::QtRasterFormat>({ KiriView::QtRasterFormat::Jxl }));
}

void TestImageDecodePipeline::routerCallsExactlyOneDecoderForClassifiedInputs()
{
    struct Case {
        KiriView::ImageInputClassification classification;
        QString expectedCall;
    };
    const QList<Case> cases {
        { classification(KiriView::ImageInputKind::Svg), QStringLiteral("svg") },
        { classification(KiriView::ImageInputKind::Apng), QStringLiteral("apng") },
        { classification(KiriView::ImageInputKind::HeifFamily), QStringLiteral("heif") },
        { classification(KiriView::ImageInputKind::Raw), QStringLiteral("raw") },
        { classification(KiriView::ImageInputKind::QtRaster, KiriView::QtRasterFormat::Png),
            QStringLiteral("qt") },
    };

    for (const Case &testCase : cases) {
        QStringList calls;
        KiriView::ImageDecodeRouter router(recordingHandlers(&calls),
            [testCase](const QByteArray &, const QString &) { return testCase.classification; });

        const KiriView::DecodedImageResult result
            = router.decode(QByteArrayLiteral("image bytes"), KiriView::ImageDecodeRequest {});

        QVERIFY(KiriView::decodedImageResultFailure(result) != nullptr);
        QCOMPARE(calls, QStringList({ testCase.expectedCall }));
    }
}

void TestImageDecodePipeline::selectedDecoderFailureDoesNotFallback()
{
    QStringList calls;
    KiriView::ImageDecodeRouterHandlers handlers = recordingHandlers(&calls);
    handlers.raw = recordingHandler(
        QStringLiteral("raw"), &calls, nullptr, nullptr, QStringLiteral("raw failed"));
    handlers.qtRaster = recordingHandler(
        QStringLiteral("qt"), &calls, nullptr, nullptr, QStringLiteral("qt fallback ran"));
    KiriView::ImageDecodeRouter router(
        std::move(handlers), [](const QByteArray &, const QString &) {
            return classification(KiriView::ImageInputKind::Raw);
        });

    const KiriView::DecodedImageResult result
        = router.decode(QByteArrayLiteral("not raw"), KiriView::ImageDecodeRequest {});

    const KiriView::DecodedImageFailure *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("raw failed"));
    QCOMPARE(calls, QStringList({ QStringLiteral("raw") }));
}

void TestImageDecodePipeline::compatibleDataIsComputedOnlyWhenClassificationRequestsIt()
{
    const QByteArray originalData = QByteArrayLiteral("original image bytes");
    const QByteArray compatibleData = QByteArrayLiteral("compatible image bytes");

    int qtCompatibleTransformCount = 0;
    QStringList qtCalls;
    QByteArrayList qtInputData;
    KiriView::ImageDecodeRouter qtRouter(
        recordingHandlers(&qtCalls, &qtInputData),
        [](const QByteArray &, const QString &) {
            return classification(
                KiriView::ImageInputKind::QtRaster, KiriView::QtRasterFormat::Png);
        },
        [&qtCompatibleTransformCount, compatibleData](const QByteArray &) {
            ++qtCompatibleTransformCount;
            return compatibleData;
        });

    qtRouter.decode(originalData, KiriView::ImageDecodeRequest {});

    QCOMPARE(qtCompatibleTransformCount, 0);
    QCOMPARE(qtCalls, QStringList({ QStringLiteral("qt") }));
    QCOMPARE(qtInputData, QByteArrayList({ originalData }));

    int heifCompatibleTransformCount = 0;
    QStringList heifCalls;
    QByteArrayList heifInputData;
    KiriView::ImageDecodeRouter heifRouter(
        recordingHandlers(&heifCalls, &heifInputData),
        [](const QByteArray &, const QString &) {
            return classification(KiriView::ImageInputKind::HeifFamily,
                KiriView::QtRasterFormat::None, KiriView::ImageDecodeDataSource::AvifCompatible);
        },
        [&heifCompatibleTransformCount, compatibleData](const QByteArray &) {
            ++heifCompatibleTransformCount;
            return compatibleData;
        });

    heifRouter.decode(originalData, KiriView::ImageDecodeRequest {});

    QCOMPARE(heifCompatibleTransformCount, 1);
    QCOMPARE(heifCalls, QStringList({ QStringLiteral("heif") }));
    QCOMPARE(heifInputData, QByteArrayList({ compatibleData }));
}

void TestImageDecodePipeline::qtRasterClassificationCarriesExplicitFormat()
{
    QStringList calls;
    QList<KiriView::QtRasterFormat> qtFormats;
    KiriView::ImageDecodeRouter router(
        recordingHandlers(&calls, nullptr, &qtFormats), [](const QByteArray &, const QString &) {
            return classification(
                KiriView::ImageInputKind::QtRaster, KiriView::QtRasterFormat::Tiff);
        });

    router.decode(QByteArrayLiteral("tiff bytes"), KiriView::ImageDecodeRequest {});

    QCOMPARE(calls, QStringList({ QStringLiteral("qt") }));
    QCOMPARE(qtFormats, QList<KiriView::QtRasterFormat>({ KiriView::QtRasterFormat::Tiff }));
}

void TestImageDecodePipeline::defaultSvgDecodeUsesFirstDisplayContext()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");
    const KiriView::DecodedImageResult result = KiriView::decodeImageDataWithDefaultRouter(data,
        KiriView::ImageDecodeRequest::fromUrl(7, QUrl(QStringLiteral("file:///tmp/vector.svg")),
            KiriView::ImageFirstDisplayDecodeContext { QSize(200, 200) }));

    const auto *image = KiriView::decodedImageResultImageAs<KiriView::StaticDecodedImage>(result);
    QVERIFY(image != nullptr);
    QCOMPARE(image->displayImage.image.size(), QSize(200, 100));
    QCOMPARE(image->displayImage.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(image->displayImage.displayPixelsPerSourcePixel, 2.5);
}

void TestImageDecodePipeline::unknownClassificationFailsWithoutDecoder()
{
    int compatibleTransformCount = 0;
    QStringList calls;
    KiriView::ImageDecodeRouter router(
        recordingHandlers(&calls),
        [](const QByteArray &, const QString &) {
            return classification(KiriView::ImageInputKind::Unknown, KiriView::QtRasterFormat::None,
                KiriView::ImageDecodeDataSource::AvifCompatible);
        },
        [&compatibleTransformCount](const QByteArray &data) {
            ++compatibleTransformCount;
            return data + QByteArrayLiteral("-compatible");
        });

    const KiriView::DecodedImageResult result
        = router.decode(QByteArrayLiteral("unknown bytes"), KiriView::ImageDecodeRequest {});

    const KiriView::DecodedImageFailure *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(
        failure->errorString, KiriView::imageErrorText(KiriView::ImageErrorTextId::ReadImageData));
    QVERIFY(calls.isEmpty());
    QCOMPARE(compatibleTransformCount, 0);
}

QTEST_GUILESS_MAIN(TestImageDecodePipeline)

#include "test_imagedecodepipeline.moc"
