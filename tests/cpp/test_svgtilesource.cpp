// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/svgtilesource.h"

#include <QColor>
#include <QObject>
#include <QRect>
#include <QRectF>
#include <QTest>
#include <Qt>
#include <memory>
#include <optional>

namespace {
QByteArray clippedSvgData()
{
    return QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" height=\"8\">"
                             "<rect width=\"12\" height=\"8\" fill=\"white\"/>"
                             "<clipPath id=\"clip\">"
                             "<rect x=\"2\" y=\"1\" width=\"4\" height=\"4\"/>"
                             "</clipPath>"
                             "<g clip-path=\"url(#clip)\">"
                             "<rect width=\"12\" height=\"8\" fill=\"red\"/>"
                             "</g>"
                             "</svg>");
}
}

class TestSvgTileSource : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceRendersIntrinsicPreviewAndTile();
    void sourceRendersUpscaledFirstDisplayPreview();
    void sourceSkipsFirstDisplayPreviewWithoutValidViewport();
    void sourceReportsFirstDisplayRenderFailure();
    void sourceAppliesClipPathToPreviewAndTile();
    void sourceRendersOversampledBucketTile();
    void sourceRejectsEmptyTileRequest();
};

void TestSvgTileSource::sourceRendersIntrinsicPreviewAndTile()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(80, 40));

    const KiriView::FirstDisplayImageDecodeResult firstDisplay = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(20, 20) }, &errorString);
    QCOMPARE(firstDisplay.status, KiriView::FirstDisplayImageDecodeStatus::Ready);
    QCOMPARE(firstDisplay.image.size(), QSize(20, 10));
    QCOMPARE(firstDisplay.displayPixelsPerSourcePixel, 0.25);

    const QImage preview = source->decodeBlockingDisplayImage(20, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.size(), QSize(20, 10));

    const KiriView::TilePyramid pyramid(source->imageSize());
    const KiriView::TileRequest request = pyramid.requestForTile(KiriView::TileKey { 0, 0, 0 });
    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.size(), QSize(80, 40));
    QVERIFY(qRed(tile->image.pixel(10, 10)) > 0);
}

void TestSvgTileSource::sourceRendersUpscaledFirstDisplayPreview()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const KiriView::FirstDisplayImageDecodeResult firstDisplay = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(200, 200) }, &errorString);

    QCOMPARE(firstDisplay.status, KiriView::FirstDisplayImageDecodeStatus::Ready);
    QCOMPARE(firstDisplay.image.size(), QSize(200, 100));
    QCOMPARE(firstDisplay.displayPixelsPerSourcePixel, 2.5);
}

void TestSvgTileSource::sourceSkipsFirstDisplayPreviewWithoutValidViewport()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const KiriView::FirstDisplayImageDecodeResult firstDisplay = source->decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext {}, &errorString);

    QCOMPARE(firstDisplay.status, KiriView::FirstDisplayImageDecodeStatus::NotImplemented);
    QVERIFY(firstDisplay.image.isNull());
}

void TestSvgTileSource::sourceReportsFirstDisplayRenderFailure()
{
    KiriView::SvgTileSource source(QByteArrayLiteral("not svg"), QSize(80, 40));

    QString errorString;
    const KiriView::FirstDisplayImageDecodeResult firstDisplay = source.decodeFirstDisplayImage(
        KiriView::ImageFirstDisplayDecodeContext { QSize(20, 20) }, &errorString);

    QCOMPARE(firstDisplay.status, KiriView::FirstDisplayImageDecodeStatus::Error);
    QVERIFY(firstDisplay.image.isNull());
    QVERIFY(!errorString.isEmpty());
}

void TestSvgTileSource::sourceAppliesClipPathToPreviewAndTile()
{
    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(clippedSvgData(), &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));
    QCOMPARE(source->imageSize(), QSize(12, 8));

    const QImage preview = source->decodeBlockingDisplayImage(12, &errorString);
    QVERIFY2(!preview.isNull(), qPrintable(errorString));
    QCOMPARE(preview.pixelColor(3, 2), QColor(Qt::red));
    QCOMPARE(preview.pixelColor(8, 2), QColor(Qt::white));

    const KiriView::TilePyramid pyramid(source->imageSize());
    const KiriView::TileRequest request = pyramid.requestForTile(KiriView::TileKey { 0, 0, 0 });
    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->image.pixelColor(3, 2), QColor(Qt::red));
    QCOMPARE(tile->image.pixelColor(8, 2), QColor(Qt::white));
}

void TestSvgTileSource::sourceRendersOversampledBucketTile()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    KiriView::TileRequest request;
    request.key = KiriView::TileKey { 0, 0, 0, 1 };
    request.levelSize = QSize(120, 60);
    request.levelRect = QRect(0, 0, 120, 60);
    request.textureLevelRect = QRect(0, 0, 120, 60);
    request.sourceRect = QRect(0, 0, 80, 40);
    request.displaySourceRect = QRect(0, 0, 80, 40);
    request.displaySourceRectF = QRectF(0.0, 0.0, 80.0 / 1.5, 40.0);

    const std::optional<KiriView::DecodedTile> tile = source->decodeTile(request, &errorString);
    QVERIFY2(tile.has_value(), qPrintable(errorString));
    QCOMPARE(tile->key.scaleBucket, 1);
    QCOMPARE(tile->image.size(), QSize(120, 60));
    QCOMPARE(tile->displaySourceRect, QRect(0, 0, 80, 40));
    QCOMPARE(tile->displaySourceRectF, QRectF(0.0, 0.0, 80.0 / 1.5, 40.0));
    QCOMPARE(tile->image.pixelColor(10, 10), QColor(Qt::red));
}

void TestSvgTileSource::sourceRejectsEmptyTileRequest()
{
    const QByteArray data
        = QByteArrayLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"40\">"
                            "<rect width=\"80\" height=\"40\" fill=\"red\"/>"
                            "</svg>");

    QString errorString;
    std::shared_ptr<KiriView::SvgTileSource> source
        = KiriView::SvgTileSource::open(data, &errorString);
    QVERIFY2(source != nullptr, qPrintable(errorString));

    const std::optional<KiriView::DecodedTile> tile
        = source->decodeTile(KiriView::TileRequest {}, &errorString);

    QVERIFY(!tile.has_value());
}

QTEST_GUILESS_MAIN(TestSvgTileSource)

#include "test_svgtilesource.moc"
