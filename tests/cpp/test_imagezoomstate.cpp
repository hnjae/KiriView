// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagezoomstate.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <limits>

namespace {
using KiriView::imageZoomApproximatelyEqual;
using KiriView::ImageZoomMode;
using KiriView::ImageZoomState;
using KiriView::LoadedImageZoom;

QUrl containerUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

class TestImageZoomState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fitModesUsePhysicalPixels();
    void manualZoomIsClampedAndRejectsInvalidValues();
    void zoomIsPreservedWithinAContainer();
    void displaySizeForZoomRejectsInvalidInputs();
    void manualZoomConstantsAreUsable();
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

void TestImageZoomState::manualZoomIsClampedAndRejectsInvalidValues()
{
    ImageZoomState state;
    state.setViewportSize(QSizeF(400.0, 300.0), 1.0);
    state.setImageSize(QSize(200, 100), 1.0);

    const qreal minimumZoomPercent = ImageZoomState::minimumManualZoomPercent();
    const qreal maximumZoomPercent = ImageZoomState::maximumManualZoomPercent();

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

void TestImageZoomState::manualZoomConstantsAreUsable()
{
    QVERIFY(ImageZoomState::minimumManualZoomPercent() > 0.0);
    QVERIFY(
        ImageZoomState::maximumManualZoomPercent() > ImageZoomState::minimumManualZoomPercent());
    QVERIFY(ImageZoomState::manualZoomStepPercent() > 0);
}

QTEST_GUILESS_MAIN(TestImageZoomState)

#include "test_imagezoomstate.moc"
