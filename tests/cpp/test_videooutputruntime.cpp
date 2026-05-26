// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videooutputruntime.h"

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QTest>

class TestVideoOutputRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void outputAttachDetachAndSameOutputAreCanonical();
    void outputDestructionClearsCanonicalOutputAndBackend();
    void geometryAndRenderContextProjectZoom();
};

namespace {
struct RuntimeFixture {
    QObject context;
    QPointer<QObject> backendOutput;
    int backendOutputSetCount = 0;
    int outputChangedCount = 0;
    int zoomProjectionChangedCount = 0;
    KiriView::VideoOutputRuntime runtime;

    RuntimeFixture()
        : runtime(&context,
              KiriView::VideoOutputRuntimeCallbacks {
                  [this](QObject *output) {
                      backendOutput = output;
                      ++backendOutputSetCount;
                  },
                  [this]() { ++outputChangedCount; },
                  [this]() { ++zoomProjectionChangedCount; },
              })
    {
    }
};
}

void TestVideoOutputRuntime::outputAttachDetachAndSameOutputAreCanonical()
{
    RuntimeFixture fixture;
    QObject output;

    fixture.runtime.setVideoOutput(&output);

    QCOMPARE(fixture.runtime.videoOutput(), &output);
    QCOMPARE(fixture.backendOutput.data(), &output);
    QCOMPARE(fixture.backendOutputSetCount, 1);
    QCOMPARE(fixture.outputChangedCount, 1);

    fixture.runtime.setVideoOutput(&output);

    QCOMPARE(fixture.runtime.videoOutput(), &output);
    QCOMPARE(fixture.backendOutput.data(), &output);
    QCOMPARE(fixture.backendOutputSetCount, 2);
    QCOMPARE(fixture.outputChangedCount, 1);

    fixture.runtime.setVideoOutput(nullptr);

    QCOMPARE(fixture.runtime.videoOutput(), nullptr);
    QCOMPARE(fixture.backendOutput.data(), nullptr);
    QCOMPARE(fixture.backendOutputSetCount, 3);
    QCOMPARE(fixture.outputChangedCount, 2);
}

void TestVideoOutputRuntime::outputDestructionClearsCanonicalOutputAndBackend()
{
    RuntimeFixture fixture;
    auto *output = new QObject();

    fixture.runtime.setVideoOutput(output);
    delete output;

    QCOMPARE(fixture.runtime.videoOutput(), nullptr);
    QCOMPARE(fixture.backendOutput.data(), nullptr);
    QCOMPARE(fixture.outputChangedCount, 2);
}

void TestVideoOutputRuntime::geometryAndRenderContextProjectZoom()
{
    RuntimeFixture fixture;
    QQuickWindow window;
    QQuickItem output;
    output.setParentItem(window.contentItem());

    fixture.runtime.setVideoOutput(&output);
    const int changedAfterOutput = fixture.zoomProjectionChangedCount;

    fixture.runtime.setVideoOutputGeometry(
        QRectF(0.0, 0.0, 1280.0, 720.0), QRectF(0.0, 0.0, 1280.0, 720.0));

    QVERIFY(fixture.runtime.zoomPercent().has_value());
    QCOMPARE(fixture.runtime.zoomPercent().value(), 100);
    QVERIFY(fixture.zoomProjectionChangedCount > changedAfterOutput);

    const int changedAfterGeometry = fixture.zoomProjectionChangedCount;
    fixture.runtime.setVideoOutputGeometry(
        QRectF(0.0, 0.0, 1280.0, 720.0), QRectF(0.0, 0.0, 1280.0, 720.0));

    QCOMPARE(fixture.zoomProjectionChangedCount, changedAfterGeometry);
}

QTEST_MAIN(TestVideoOutputRuntime)

#include "test_videooutputruntime.moc"
