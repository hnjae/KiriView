// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/apngrgbabuffer.h"

#include <QColor>
#include <QObject>
#include <QTest>
#include <array>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <vector>

namespace {
using Pixel = std::array<unsigned char, 4>;

Pixel rgba(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
    return Pixel { red, green, blue, alpha };
}

void writePixels(unsigned char *row, std::initializer_list<Pixel> pixels)
{
    std::memcpy(row, pixels.begin(), pixels.size() * sizeof(Pixel));
}

QColor pixel(const QImage &image, int x, int y) { return image.pixelColor(x, y); }
}

class TestApngRgbaBuffer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initializeRejectsInvalidLayout();
    void rowsExposePaddedRgbaStorage();
    void regionCopyClearAndRestoreUseCanvasOffsets();
    void imageCopyOwnsTheCurrentBufferBytes();
};

void TestApngRgbaBuffer::initializeRejectsInvalidLayout()
{
    kiriview::ApngRgbaBuffer buffer;

    QVERIFY(!buffer.initialize(QSize(), 0));
    QVERIFY(!buffer.isValid());
    QVERIFY(!buffer.initialize(QSize(2, 1), 7));
    QVERIFY(!buffer.isValid());

    QVERIFY(buffer.initialize(QSize(2, 1), 8));
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.imageSize(), QSize(2, 1));
    QCOMPARE(buffer.rowBytes(), std::size_t(8));
}

void TestApngRgbaBuffer::rowsExposePaddedRgbaStorage()
{
    kiriview::ApngRgbaBuffer buffer;
    QVERIFY(buffer.initialize(QSize(1, 2), 8));

    QVERIFY(buffer.rows() != nullptr);
    QVERIFY(buffer.row(0) != nullptr);
    QVERIFY(buffer.row(1) != nullptr);
    QCOMPARE(buffer.row(1), buffer.row(0) + 8);
}

void TestApngRgbaBuffer::regionCopyClearAndRestoreUseCanvasOffsets()
{
    kiriview::ApngRgbaBuffer buffer;
    QVERIFY(buffer.initialize(QSize(2, 2), 8));
    writePixels(buffer.row(0), { rgba(255, 0, 0, 255), rgba(0, 255, 0, 255) });
    writePixels(buffer.row(1), { rgba(0, 0, 255, 255), rgba(255, 255, 0, 255) });

    const kiriview::ApngRgbaRegion rightColumn { 1, 2, 1, 0 };
    QVERIFY(buffer.contains(rightColumn));
    const std::optional<std::vector<unsigned char>> copied = buffer.copyRegion(rightColumn);
    QVERIFY(copied.has_value());
    QCOMPARE(copied->size(), std::size_t(8));

    QVERIFY(buffer.clearRegion(rightColumn));
    const std::optional<QImage> cleared = buffer.imageCopy();
    QVERIFY(cleared.has_value());
    QCOMPARE(pixel(*cleared, 0, 0), QColor(255, 0, 0, 255));
    QCOMPARE(pixel(*cleared, 1, 0).alpha(), 0);
    QCOMPARE(pixel(*cleared, 1, 1).alpha(), 0);

    QVERIFY(buffer.restoreRegion(rightColumn, *copied));
    const std::optional<QImage> restored = buffer.imageCopy();
    QVERIFY(restored.has_value());
    QCOMPARE(pixel(*restored, 1, 0), QColor(0, 255, 0, 255));
    QCOMPARE(pixel(*restored, 1, 1), QColor(255, 255, 0, 255));

    QVERIFY(!buffer.contains(kiriview::ApngRgbaRegion { 2, 1, 1, 0 }));
    QVERIFY(!buffer.copyRegion(kiriview::ApngRgbaRegion { 2, 1, 1, 0 }).has_value());
}

void TestApngRgbaBuffer::imageCopyOwnsTheCurrentBufferBytes()
{
    kiriview::ApngRgbaBuffer buffer;
    QVERIFY(buffer.initialize(QSize(1, 1), 4));
    writePixels(buffer.row(0), { rgba(255, 0, 0, 255) });

    const std::optional<QImage> image = buffer.imageCopy();
    QVERIFY(image.has_value());
    writePixels(buffer.row(0), { rgba(0, 0, 255, 255) });

    QCOMPARE(pixel(*image, 0, 0), QColor(255, 0, 0, 255));
}

QTEST_GUILESS_MAIN(TestApngRgbaBuffer)

#include "test_apngrgbabuffer.moc"
