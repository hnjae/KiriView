// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodepipeline.h"

#include "localization/imageerrortext.h"

#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QObject>
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
    void routerCallsExactlyOneDecoderForClassifiedInputs();
    void selectedDecoderFailureDoesNotFallback();
    void compatibleDataIsComputedOnlyWhenClassificationRequestsIt();
    void qtRasterClassificationCarriesExplicitFormat();
    void unknownClassificationFailsWithoutDecoder();
};

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

void TestImageDecodePipeline::unknownClassificationFailsWithoutDecoder()
{
    int compatibleTransformCount = 0;
    QStringList calls;
    KiriView::ImageDecodeRouter router(
        recordingHandlers(&calls),
        [](const QByteArray &, const QString &) {
            return classification(KiriView::ImageInputKind::Unknown);
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
