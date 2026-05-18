// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apngframecomposer.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QTest>
#include <array>
#include <cstring>
#include <initializer_list>
#include <optional>

namespace {
using Pixel = std::array<unsigned char, 4>;

Pixel rgba(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
    return Pixel { red, green, blue, alpha };
}

KiriView::ApngFrameControl frameControl(quint32 width, quint32 height)
{
    KiriView::ApngFrameControl control;
    control.width = width;
    control.height = height;
    return control;
}

void writeFramePixels(KiriView::ApngFrameComposer *composer, std::initializer_list<Pixel> pixels)
{
    unsigned char **rows = composer->frameRows();
    Q_ASSERT(rows != nullptr);
    std::memcpy(rows[0], pixels.begin(), pixels.size() * sizeof(Pixel));
}

QColor pixel(const QImage &image, int x, int y) { return image.pixelColor(x, y); }
}

class TestApngFrameComposer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rejectsFramesOutsideTheCanvas();
    void firstFrameWithDisposePreviousClearsAsBackground();
    void blendOverComposesWithExistingCanvas();
    void disposePreviousRestoresCanvasForFollowingFrames();
};

void TestApngFrameComposer::rejectsFramesOutsideTheCanvas()
{
    KiriView::ApngFrameComposer composer;
    QVERIFY(!composer.canComposeFrame(frameControl(1, 1)));
    QVERIFY(composer.initialize(QSize(2, 1), 8));
    QVERIFY(composer.canComposeFrame(frameControl(2, 1)));

    KiriView::ApngFrameControl outside = frameControl(2, 1);
    outside.xOffset = 1;
    QVERIFY(!composer.canComposeFrame(outside));
    QVERIFY(!composer.composeFrame(outside).has_value());
}

void TestApngFrameComposer::firstFrameWithDisposePreviousClearsAsBackground()
{
    KiriView::ApngFrameComposer composer;
    QVERIFY(composer.initialize(QSize(2, 1), 8));

    KiriView::ApngFrameControl first = frameControl(2, 1);
    first.disposeOp = KiriView::ApngFrameDisposeOp::Previous;
    writeFramePixels(&composer, { rgba(255, 0, 0, 255), rgba(255, 0, 0, 255) });
    const std::optional<QImage> firstImage = composer.composeFrame(first);
    QVERIFY(firstImage.has_value());
    QCOMPARE(pixel(*firstImage, 0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(pixel(*firstImage, 1, 0), QColor(255, 0, 0, 255));

    KiriView::ApngFrameControl second = frameControl(1, 1);
    second.xOffset = 1;
    writeFramePixels(&composer, { rgba(0, 255, 0, 255) });
    const std::optional<QImage> secondImage = composer.composeFrame(second);
    QVERIFY(secondImage.has_value());
    QCOMPARE(pixel(*secondImage, 0, 0).alpha(), 0);
    QCOMPARE(pixel(*secondImage, 1, 0), QColor(0, 255, 0, 255));
}

void TestApngFrameComposer::blendOverComposesWithExistingCanvas()
{
    KiriView::ApngFrameComposer composer;
    QVERIFY(composer.initialize(QSize(1, 1), 4));

    writeFramePixels(&composer, { rgba(255, 0, 0, 255) });
    QVERIFY(composer.composeFrame(frameControl(1, 1)).has_value());

    KiriView::ApngFrameControl over = frameControl(1, 1);
    over.blendOp = KiriView::ApngFrameBlendOp::Over;
    writeFramePixels(&composer, { rgba(0, 0, 255, 128) });
    const std::optional<QImage> image = composer.composeFrame(over);
    QVERIFY(image.has_value());
    const QColor color = pixel(*image, 0, 0);
    QCOMPARE(color.alpha(), 255);
    QVERIFY(color.red() > 0);
    QVERIFY(color.blue() > 0);
}

void TestApngFrameComposer::disposePreviousRestoresCanvasForFollowingFrames()
{
    KiriView::ApngFrameComposer composer;
    QVERIFY(composer.initialize(QSize(2, 1), 8));

    writeFramePixels(&composer, { rgba(255, 0, 0, 255), rgba(255, 0, 0, 255) });
    QVERIFY(composer.composeFrame(frameControl(2, 1)).has_value());

    KiriView::ApngFrameControl temporary = frameControl(1, 1);
    temporary.disposeOp = KiriView::ApngFrameDisposeOp::Previous;
    writeFramePixels(&composer, { rgba(0, 0, 255, 255) });
    const std::optional<QImage> temporaryImage = composer.composeFrame(temporary);
    QVERIFY(temporaryImage.has_value());
    QCOMPARE(pixel(*temporaryImage, 0, 0), QColor(0, 0, 255, 255));

    KiriView::ApngFrameControl next = frameControl(1, 1);
    next.xOffset = 1;
    writeFramePixels(&composer, { rgba(0, 255, 0, 255) });
    const std::optional<QImage> nextImage = composer.composeFrame(next);
    QVERIFY(nextImage.has_value());
    QCOMPARE(pixel(*nextImage, 0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(pixel(*nextImage, 1, 0), QColor(0, 255, 0, 255));
}

QTEST_GUILESS_MAIN(TestApngFrameComposer)

#include "test_apngframecomposer.moc"
