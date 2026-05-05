// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimageresult.h"

#include <QImage>
#include <QObject>
#include <QTest>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace {
QImage testImage(const QSize &size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

class ResultTileSource final : public KiriView::ImageTileSource
{
public:
    explicit ResultTileSource(QSize imageSize = QSize(16, 16))
        : m_imageSize(std::move(imageSize))
    {
    }

    QSize imageSize() const override { return m_imageSize; }
    qsizetype byteCost() const override { return 1; }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &, QString *) const override
    {
        return std::nullopt;
    }

    KiriView::FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const KiriView::ImageFirstDisplayDecodeContext &, QString *errorString) const override
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

    KiriView::FirstDisplayImageDecodeResult firstDisplayResult;
    QImage blockingDisplay;
    QString firstDisplayError;
    QString blockingDisplayError;
    mutable int firstDisplayDecodeCount = 0;
    mutable int blockingDisplayDecodeCount = 0;
    mutable int blockingDisplayMaximumLongEdge = 0;

private:
    QSize m_imageSize;
};

const KiriView::StaticDecodedImage *staticDecodedImage(const KiriView::DecodedImageResult &result)
{
    const KiriView::DecodedImage *image = KiriView::decodedImageResultImage(result);
    return image == nullptr ? nullptr : std::get_if<KiriView::StaticDecodedImage>(image);
}
}

class TestDecodedImageResult : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staticResultUsesReadyFirstDisplayImage();
    void staticResultFallsBackToBlockingPreview();
    void staticResultReportsFirstDisplayErrors();
};

void TestDecodedImageResult::staticResultUsesReadyFirstDisplayImage()
{
    auto source = std::make_shared<ResultTileSource>();
    source->firstDisplayResult = KiriView::FirstDisplayImageDecodeResult {
        KiriView::FirstDisplayImageDecodeStatus::Ready,
        testImage(QSize(8, 4)),
        0.5,
    };

    QString errorString;
    const KiriView::DecodedImageResult result = KiriView::staticDecodedImageResult(
        source, KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    const KiriView::StaticDecodedImage *decoded = staticDecodedImage(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->staticImage.source.get(), source.get());
    QCOMPARE(decoded->staticImage.preview.size(), QSize(8, 4));
    QCOMPARE(decoded->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 0);
    QVERIFY(errorString.isEmpty());
}

void TestDecodedImageResult::staticResultFallsBackToBlockingPreview()
{
    auto source = std::make_shared<ResultTileSource>();
    source->blockingDisplay = testImage(QSize(12, 9));

    QString errorString;
    const KiriView::DecodedImageResult result = KiriView::staticDecodedImageResult(
        source, KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    const KiriView::StaticDecodedImage *decoded = staticDecodedImage(result);
    QVERIFY(decoded != nullptr);
    QCOMPARE(decoded->staticImage.source.get(), source.get());
    QCOMPARE(decoded->staticImage.preview.size(), QSize(12, 9));
    QCOMPARE(decoded->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.0);
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayMaximumLongEdge, KiriView::imageBlockingDisplayLongEdgeMax);
    QVERIFY(errorString.isEmpty());
}

void TestDecodedImageResult::staticResultReportsFirstDisplayErrors()
{
    auto source = std::make_shared<ResultTileSource>();
    source->firstDisplayResult.status = KiriView::FirstDisplayImageDecodeStatus::Error;
    source->firstDisplayError = QStringLiteral("first display failed");
    source->blockingDisplay = testImage(QSize(12, 9));

    QString errorString;
    const KiriView::DecodedImageResult result = KiriView::staticDecodedImageResult(
        source, KiriView::ImageFirstDisplayDecodeContext { QSize(400, 300) }, &errorString);

    const KiriView::DecodedImageFailure *failure = KiriView::decodedImageResultFailure(result);
    QVERIFY(failure != nullptr);
    QCOMPARE(failure->errorString, QStringLiteral("first display failed"));
    QCOMPARE(source->firstDisplayDecodeCount, 1);
    QCOMPARE(source->blockingDisplayDecodeCount, 0);
}

QTEST_GUILESS_MAIN(TestDecodedImageResult)

#include "test_decodedimageresult.moc"
