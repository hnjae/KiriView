// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videozoomstate.h"

#include <QObject>
#include <QRectF>
#include <QTest>
#include <optional>

class TestVideoZoomState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void usesEffectiveDevicePixelRatioForPhysicalDisplaySize();
    void usesFittedAxisForLetterboxedContent();
    void rejectsInvalidInputs();
};

void TestVideoZoomState::usesEffectiveDevicePixelRatioForPhysicalDisplaySize()
{
    const qreal devicePixelRatio = 2.075;
    const QRectF contentRect(0.0, 0.0, 3840.0 / devicePixelRatio, 2160.0 / devicePixelRatio);
    const QRectF sourceRect(0.0, 0.0, 2560.0, 1440.0);

    const std::optional<int> zoomPercent
        = kiriview::videoZoomPercentForRects(contentRect, sourceRect, devicePixelRatio);

    QVERIFY(zoomPercent.has_value());
    QCOMPARE(zoomPercent.value(), 150);
}

void TestVideoZoomState::usesFittedAxisForLetterboxedContent()
{
    const QRectF contentRect(0.0, 0.0, 1200.0, 800.0);
    const QRectF sourceRect(0.0, 0.0, 1200.0, 1200.0);

    const std::optional<int> zoomPercent
        = kiriview::videoZoomPercentForRects(contentRect, sourceRect, 1.0);

    QVERIFY(zoomPercent.has_value());
    QCOMPARE(zoomPercent.value(), 67);
}

void TestVideoZoomState::rejectsInvalidInputs()
{
    QVERIFY(!kiriview::videoZoomPercentForRects(
        QRectF(0.0, 0.0, 0.0, 100.0), QRectF(0.0, 0.0, 100.0, 100.0), 1.0)
            .has_value());
    QVERIFY(!kiriview::videoZoomPercentForRects(
        QRectF(0.0, 0.0, 100.0, 100.0), QRectF(0.0, 0.0, 0.0, 100.0), 1.0)
            .has_value());
    QVERIFY(!kiriview::videoZoomPercentForRects(
        QRectF(0.0, 0.0, 100.0, 100.0), QRectF(0.0, 0.0, 100.0, 100.0), 0.0)
            .has_value());
}

QTEST_GUILESS_MAIN(TestVideoZoomState)

#include "test_videozoomstate.moc"
