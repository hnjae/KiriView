// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/staticimagedecode.h"

#include "decoding/imagedecoderequest.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>

namespace {
QImage testImage(const QSize &size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

class ResultTileSource final : public kiriview::ImageTileSource
{
public:
    explicit ResultTileSource(QSize imageSize = QSize(16, 16))
        : m_imageSize(std::move(imageSize))
    {
    }

    QSize imageSize() const override { return m_imageSize; }
    qsizetype byteCost() const override { return 1; }

    std::optional<kiriview::DecodedTile> decodeTile(
        const kiriview::TileRequest &, QString *) const override
    {
        return std::nullopt;
    }

    kiriview::FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const kiriview::ImageFirstDisplayDecodeContext &, QString *errorString) const override
    {
        ++firstDisplayDecodeCount;
        if (!firstDisplayError.isEmpty() && errorString != nullptr) {
            *errorString = firstDisplayError;
        }
        return firstDisplayResult;
    }

    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override
    {
        ++blockingDisplayDecodeCount;
        blockingDisplayMaximumLongEdge = maximumLongEdge;
        if (!blockingDisplayError.isEmpty() && errorString != nullptr) {
            *errorString = blockingDisplayError;
        }
        return blockingDisplay;
    }

    kiriview::FirstDisplayImageDecodeResult firstDisplayResult;
    QImage blockingDisplay;
    QString firstDisplayError;
    QString blockingDisplayError;
    mutable int firstDisplayDecodeCount = 0;
    mutable int blockingDisplayDecodeCount = 0;
    mutable int blockingDisplayMaximumLongEdge = 0;

private:
    QSize m_imageSize;
};

const kiriview::StaticDecodedImage *staticDecodedImage(const kiriview::DecodedImageResult &result)
{
    return kiriview::decodedImageResultImageAs<kiriview::StaticDecodedImage>(result);
}

kiriview::ImageDecodeRequest testDecodeRequest(
    kiriview::ImageFirstDisplayDecodeContext firstDisplay = {})
{
    return kiriview::ImageDecodeRequest::fromUrl(
        7, QUrl::fromLocalFile(QStringLiteral("/tmp/stage3-source.jpg")), std::move(firstDisplay));
}
}

class TestStaticImageDecode : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticResultUsesReadyFirstDisplayImage();
    void staticResultFallsBackToBlockingPreview();
    void staticResultMarksScaledBlockingPreviewAsFirstDisplayQuality();
    void staticResultReportsFirstDisplayErrors();
    void staticResultReportsMissingReadyFirstDisplayImage();
    void staticResultReportsMissingBlockingPreview();
};

void TestStaticImageDecode::staticResultUsesReadyFirstDisplayImage()
{
    auto source = std::make_shared<ResultTileSource>();
    source->firstDisplayResult = kiriview::FirstDisplayImageDecodeResult {
        kiriview::FirstDisplayImageDecodeStatus::Ready,
        testImage(QSize(8, 4)),
        0.5,
    };

    QString errorString;
    const kiriview::DecodedImageResult result = kiriview::staticDecodedImageResult(source,
        testDecodeRequest(kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }),
        &errorString);

    const kiriview::StaticDecodedImage *decoded = staticDecodedImage(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->displayImage.refinementSource.get(), source.get());
    QCOMPARE(decoded->displayImage.image.size(), QSize(8, 4));
    QCOMPARE(decoded->displayImage.originalSize, QSize(16, 16));
    QCOMPARE(decoded->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(decoded->displayImage.displayPixelsPerSourcePixel, 0.5);
    QCOMPARE(decoded->displayImage.sourceIdentity, QStringLiteral("file:///tmp/stage3-source.jpg"));
    QVERIFY(decoded->displayImage.isValid());
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 0);
    QVERIFY(errorString.isEmpty());
}

void TestStaticImageDecode::staticResultFallsBackToBlockingPreview()
{
    auto source = std::make_shared<ResultTileSource>(QSize(12, 9));
    source->blockingDisplay = testImage(QSize(12, 9));

    QString errorString;
    const kiriview::DecodedImageResult result = kiriview::staticDecodedImageResult(source,
        testDecodeRequest(kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }),
        &errorString);

    const kiriview::StaticDecodedImage *decoded = staticDecodedImage(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->displayImage.refinementSource.get(), source.get());
    QCOMPARE(decoded->displayImage.image.size(), QSize(12, 9));
    QCOMPARE(decoded->displayImage.originalSize, QSize(12, 9));
    QCOMPARE(decoded->displayImage.quality, kiriview::DisplayImageQuality::Exact);
    QCOMPARE(decoded->displayImage.displayPixelsPerSourcePixel, 1.0);
    QCOMPARE(decoded->displayImage.previewOrigin, kiriview::DisplayImagePreviewOrigin::None);
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayMaximumLongEdge, kiriview::imageBlockingDisplayLongEdgeMax);
    QVERIFY(errorString.isEmpty());
}

void TestStaticImageDecode::staticResultMarksScaledBlockingPreviewAsFirstDisplayQuality()
{
    auto source = std::make_shared<ResultTileSource>(QSize(4000, 3000));
    source->blockingDisplay = testImage(QSize(2048, 1536));

    QString errorString;
    const kiriview::DecodedImageResult result
        = kiriview::staticDecodedImageResult(source, testDecodeRequest(), &errorString);

    const kiriview::StaticDecodedImage *decoded = staticDecodedImage(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->displayImage.image.size(), QSize(2048, 1536));
    QCOMPARE(decoded->displayImage.originalSize, QSize(4000, 3000));
    QCOMPARE(decoded->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QVERIFY(decoded->displayImage.displayPixelsPerSourcePixel > 0.0);
    QVERIFY(decoded->displayImage.displayPixelsPerSourcePixel < 1.0);
    QVERIFY(errorString.isEmpty());
}

void TestStaticImageDecode::staticResultReportsFirstDisplayErrors()
{
    auto source = std::make_shared<ResultTileSource>();
    source->firstDisplayResult.status = kiriview::FirstDisplayImageDecodeStatus::Error;
    source->firstDisplayError = QStringLiteral("first display failed");
    source->blockingDisplay = testImage(QSize(12, 9));

    QString errorString;
    const kiriview::DecodedImageResult result = kiriview::staticDecodedImageResult(source,
        testDecodeRequest(kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }),
        &errorString);

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("first display failed"));
    QCOMPARE(failure->operation, kiriview::DecodedImageFailureOperation::DecodeFirstDisplayImage);
    QCOMPARE(failure->diagnosticDetail, QStringLiteral("first display failed"));
    QCOMPARE(failure->severity, kiriview::DecodedImageFailureSeverity::Error);
    QVERIFY(!failure->retryable);
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 0);
}

void TestStaticImageDecode::staticResultReportsMissingReadyFirstDisplayImage()
{
    auto source = std::make_shared<ResultTileSource>();
    source->firstDisplayResult.status = kiriview::FirstDisplayImageDecodeStatus::Ready;
    source->blockingDisplay = testImage(QSize(12, 9));

    QString errorString;
    const kiriview::DecodedImageResult result = kiriview::staticDecodedImageResult(source,
        testDecodeRequest(kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }),
        &errorString);

    QVERIFY(kiriview::decodedImageResultFailure(result) != nullptr);
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 0);
}

void TestStaticImageDecode::staticResultReportsMissingBlockingPreview()
{
    auto source = std::make_shared<ResultTileSource>();
    source->blockingDisplayError = QStringLiteral("blocking display failed");

    QString errorString;
    const kiriview::DecodedImageResult result = kiriview::staticDecodedImageResult(source,
        testDecodeRequest(kiriview::ImageFirstDisplayDecodeContext { QSize(400, 300) }),
        &errorString);

    const kiriview::DecodedImageFailure *failure = kiriview::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("blocking display failed"));
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 1);
}

QTEST_GUILESS_MAIN(TestStaticImageDecode)

#include "test_staticimagedecode.moc"
