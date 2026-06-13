// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagezoomstate.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
using kiriview::imageZoomApproximatelyEqual;
using kiriview::ImageZoomMode;
using kiriview::ImageZoomState;
using kiriview::LoadedImageZoom;

QUrl containerUrl(const QString &path) { return QUrl::fromLocalFile(path); }

qreal longEdge(const QSizeF &size) { return std::max(size.width(), size.height()); }

qreal binaryOctaveZoomStepFactor() { return std::pow(2.0, 1.0 / 8.0); }
}

class TestImageZoomState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fitModesUsePhysicalPixels();
    void manualZoomIsClampedByDynamicMaximumAndRejectsInvalidValues();
    void smallImagesCanZoomPastLegacyFixedMaximum();
    void largeImagesClampToLogicalDisplayLongEdge();
    void viewportGrowthIncreasesManualMaximum();
    void higherDevicePixelRatioPreservesLogicalDisplayCap();
    void zoomIsPreservedWithinAContainer();
    void displaySizeForZoomRejectsInvalidInputs();
    void manualZoomStepsUseMultiplicativeFactor();
    void manualZoomLimitsAreUsable();
    void manualZoomPercentPropertyValueIsBoundedInteger();
};

void TestImageZoomState::fitModesUsePhysicalPixels()
{
    ImageZoomState state;

    QVERIFY(state.setViewportSize(QSizeF(400.0, 300.0), 1.0));
    QVERIFY(state.setImageSize(QSize(200, 100), 1.0));

    QCOMPARE(state.zoomMode(), ImageZoomMode::Fit);
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), 200.0));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(), QSizeF(400.0, 200.0)));

    QVERIFY(state.setFitMode(ImageZoomMode::FitHeight, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), 300.0));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(), QSizeF(600.0, 300.0)));

    QVERIFY(state.setFitMode(ImageZoomMode::FitWidth, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), 200.0));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(), QSizeF(400.0, 200.0)));

    state.update(2.0);
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), 400.0));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(), QSizeF(400.0, 200.0)));
}

void TestImageZoomState::manualZoomIsClampedByDynamicMaximumAndRejectsInvalidValues()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.setImageSize(QSize(200, 100), 1.0);

    const qreal minimumZoomPercent = ImageZoomState::minimumManualZoomPercent();
    const qreal maximumZoomPercent = state.maximumManualZoomPercent(1.0);

    QVERIFY(state.setManualZoomPercent(minimumZoomPercent - 5.0, 1.0));
    QCOMPARE(state.zoomMode(), ImageZoomMode::Manual);
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), minimumZoomPercent));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(),
        QSizeF(200.0 * minimumZoomPercent / 100.0, 100.0 * minimumZoomPercent / 100.0)));

    QVERIFY(state.setManualZoomPercent(maximumZoomPercent + 100.0, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), maximumZoomPercent));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(),
        QSizeF(200.0 * maximumZoomPercent / 100.0, 100.0 * maximumZoomPercent / 100.0)));

    const auto previous = state.snapshot();
    QVERIFY(!state.setManualZoomPercent(std::numeric_limits<qreal>::quiet_NaN(), 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), previous.zoomPercent));
    QVERIFY(imageZoomApproximatelyEqual(state.displaySize(), previous.displaySize));
}

void TestImageZoomState::smallImagesCanZoomPastLegacyFixedMaximum()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(1000.0, 750.0), 1.0);
    state.setImageSize(QSize(1, 1), 1.0);

    const qreal fitZoomPercent = state.fitZoomPercent(ImageZoomMode::Fit, 1.0);
    const qreal maximumZoomPercent = state.maximumManualZoomPercent(1.0);

    QVERIFY(maximumZoomPercent > 800.0);
    QVERIFY(maximumZoomPercent >= fitZoomPercent);
    QVERIFY(state.setManualZoomPercent(fitZoomPercent, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), fitZoomPercent));
    QVERIFY(longEdge(state.displaySize()) >= 750.0);
}

void TestImageZoomState::largeImagesClampToLogicalDisplayLongEdge()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.setImageSize(QSize(200000, 100000), 1.0);

    const qreal maximumZoomPercent = state.maximumManualZoomPercent(1.0);
    QVERIFY(maximumZoomPercent < 800.0);
    QVERIFY(imageZoomApproximatelyEqual(maximumZoomPercent, 65536.0 * 100.0 / 200000.0));

    QVERIFY(state.setManualZoomPercent(800.0, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.zoomPercent(), maximumZoomPercent));
    QVERIFY(imageZoomApproximatelyEqual(longEdge(state.displaySize()), 65536.0));
}

void TestImageZoomState::viewportGrowthIncreasesManualMaximum()
{
    ImageZoomState state;
    state.setImageSize(QSize(1000, 1000), 1.0);
    state.setViewportSize(QSizeF(7000.0, 100.0), 1.0);

    const qreal smallerViewportMaximum = state.maximumManualZoomPercent(1.0);
    QVERIFY(imageZoomApproximatelyEqual(smallerViewportMaximum, 65536.0 * 100.0 / 1000.0));

    QVERIFY(state.setViewportSize(QSizeF(9000.0, 100.0), 1.0));
    const qreal largerViewportMaximum = state.maximumManualZoomPercent(1.0);
    QVERIFY(largerViewportMaximum > smallerViewportMaximum);
    QVERIFY(imageZoomApproximatelyEqual(largerViewportMaximum, 9000.0 * 8.0 * 100.0 / 1000.0));
}

void TestImageZoomState::higherDevicePixelRatioPreservesLogicalDisplayCap()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(9000.0, 100.0), 1.0);
    state.setImageSize(QSize(1000, 1000), 1.0);

    const qreal oneXMaximum = state.maximumManualZoomPercent(1.0);
    const qreal twoXMaximum = state.maximumManualZoomPercent(2.0);
    QVERIFY(imageZoomApproximatelyEqual(twoXMaximum, oneXMaximum * 2.0));

    const QSizeF oneXDisplaySize = state.displaySizeForZoomPercent(oneXMaximum, 1.0);
    const QSizeF twoXDisplaySize = state.displaySizeForZoomPercent(twoXMaximum, 2.0);
    QVERIFY(imageZoomApproximatelyEqual(longEdge(oneXDisplaySize), 72000.0));
    QVERIFY(imageZoomApproximatelyEqual(longEdge(twoXDisplaySize), longEdge(oneXDisplaySize)));
}

void TestImageZoomState::zoomIsPreservedWithinAContainer()
{
    ImageZoomState state;
    const QUrl firstContainer = containerUrl(QStringLiteral("/images/one/"));
    const QUrl secondContainer = containerUrl(QStringLiteral("/images/two/"));

    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.prepareImageContainer(firstContainer);
    state.setImageSize(QSize(200, 100), 1.0);
    QVERIFY(state.setManualZoomPercent(150.0, 1.0));

    const LoadedImageZoom sameContainerZoom
        = state.loadedImageZoom(firstContainer, QSize(400, 200), 1.0);
    QCOMPARE(sameContainerZoom.zoomMode, ImageZoomMode::Manual);
    QVERIFY(imageZoomApproximatelyEqual(sameContainerZoom.zoomPercent, 150.0));
    QVERIFY(imageZoomApproximatelyEqual(sameContainerZoom.displaySize, QSizeF(600.0, 300.0)));

    state.prepareImageContainer(firstContainer);
    QCOMPARE(state.zoomMode(), ImageZoomMode::Manual);

    state.prepareImageContainer(secondContainer);
    QCOMPARE(state.zoomMode(), ImageZoomMode::Fit);
}

void TestImageZoomState::displaySizeForZoomRejectsInvalidInputs()
{
    ImageZoomState state;

    QVERIFY(imageZoomApproximatelyEqual(
        state.displaySizeForZoomPercent(100.0, QSize(200, 100), 2.0), QSizeF(100.0, 50.0)));
    QVERIFY(state.displaySizeForZoomPercent(100.0, QSize(200, 100), 0.0).isEmpty());
    QVERIFY(state.displaySizeForZoomPercent(100.0, QSize(200, 100), -1.0).isEmpty());
    QVERIFY(state
            .displaySizeForZoomPercent(
                100.0, QSize(200, 100), std::numeric_limits<qreal>::quiet_NaN())
            .isEmpty());
    QVERIFY(state.displaySizeForZoomPercent(0.0, QSize(200, 100), 1.0).isEmpty());
    QVERIFY(state.displaySizeForZoomPercent(100.0, QSize(), 1.0).isEmpty());
}

void TestImageZoomState::manualZoomStepsUseMultiplicativeFactor()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.setImageSize(QSize(200, 100), 1.0);

    const qreal stepFactor = binaryOctaveZoomStepFactor();
    QVERIFY(imageZoomApproximatelyEqual(ImageZoomState::manualZoomStepFactor(), stepFactor));

    QVERIFY(state.setManualZoomPercent(100.0, 1.0));
    QVERIFY(
        imageZoomApproximatelyEqual(state.steppedManualZoomPercent(1.0, 1.0), 100.0 * stepFactor));
    QVERIFY(imageZoomApproximatelyEqual(
        state.steppedManualZoomPercent(2.0, 1.0), 100.0 * std::pow(2.0, 1.0 / 4.0)));
    QVERIFY(imageZoomApproximatelyEqual(
        state.steppedManualZoomPercent(0.5, 1.0), 100.0 * std::pow(2.0, 1.0 / 16.0)));

    QVERIFY(state.setManualZoomPercent(100.0 * stepFactor, 1.0));
    QVERIFY(imageZoomApproximatelyEqual(state.steppedManualZoomPercent(-1.0, 1.0), 100.0));

    const qreal minimumZoomPercent = ImageZoomState::minimumManualZoomPercent();
    QVERIFY(state.setManualZoomPercent(minimumZoomPercent, 1.0));
    QVERIFY(
        imageZoomApproximatelyEqual(state.steppedManualZoomPercent(-1.0, 1.0), minimumZoomPercent));

    const qreal maximumZoomPercent = state.maximumManualZoomPercent(1.0);
    QVERIFY(state.setManualZoomPercent(maximumZoomPercent, 1.0));
    QVERIFY(
        imageZoomApproximatelyEqual(state.steppedManualZoomPercent(1.0, 1.0), maximumZoomPercent));
}

void TestImageZoomState::manualZoomLimitsAreUsable()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.setImageSize(QSize(200, 100), 1.0);

    QVERIFY(ImageZoomState::minimumManualZoomPercent() > 0.0);
    QVERIFY(state.maximumManualZoomPercent(1.0) > ImageZoomState::minimumManualZoomPercent());
    QVERIFY(ImageZoomState::manualZoomStepFactor() > 1.0);
}

void TestImageZoomState::manualZoomPercentPropertyValueIsBoundedInteger()
{
    QCOMPARE(ImageZoomState::manualZoomPercentPropertyValue(10.0), 10);
    QCOMPARE(ImageZoomState::manualZoomPercentPropertyValue(10.1), 11);
    QCOMPARE(ImageZoomState::manualZoomPercentPropertyValue(-4.5), 0);
    QCOMPARE(ImageZoomState::manualZoomPercentPropertyValue(std::numeric_limits<qreal>::infinity()),
        static_cast<int>(ImageZoomState::minimumManualZoomPercent()));
}

QTEST_GUILESS_MAIN(TestImageZoomState)

#include "test_imagezoomstate.moc"
