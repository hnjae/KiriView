// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagezoomworkflowstate.h"

#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>

class TestImageZoomWorkflowState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mutationOwnsRenderContextRefreshAndZoomChanges();
    void forcedTileRefreshSchedulesDecodeWithoutZoomChange();
    void clearResetsCanonicalZoomState();
};

void TestImageZoomWorkflowState::mutationOwnsRenderContextRefreshAndZoomChanges()
{
    qreal devicePixelRatio = 1.0;
    KiriView::ImageZoomWorkflowState state([&devicePixelRatio]() {
        return KiriView::ImageDocumentRenderContext {
            devicePixelRatio,
            KiriView::fallbackTextureSizeMax,
        };
    });

    KiriView::ImageZoomWorkflowMutationResult result
        = state.mutate([](KiriView::ImageZoomState &zoomState, qreal mutationDevicePixelRatio) {
              zoomState.setViewportSize(QSizeF(400.0, 200.0), mutationDevicePixelRatio);
              zoomState.setImageSize(QSize(800, 400), mutationDevicePixelRatio);
          });

    QCOMPARE(result.renderContextChange.previous.devicePixelRatio, 1.0);
    QCOMPARE(result.renderContextChange.current.devicePixelRatio, 1.0);
    QVERIFY(result.changes.imageSizeChanged);
    QVERIFY(result.changes.viewportSizeChanged);
    QVERIFY(result.changes.displaySizeChanged);
    QVERIFY(result.changes.scheduleVisibleTileDecode);
    QCOMPARE(state.zoomState().displaySize(), QSizeF(400.0, 200.0));

    devicePixelRatio = 2.0;
    result = state.mutate([](KiriView::ImageZoomState &zoomState, qreal mutationDevicePixelRatio) {
        zoomState.update(mutationDevicePixelRatio);
    });

    QCOMPARE(result.renderContextChange.previous.devicePixelRatio, 1.0);
    QCOMPARE(result.renderContextChange.current.devicePixelRatio, 2.0);
    QVERIFY(result.changes.zoomPercentChanged);
    QVERIFY(result.changes.maximumManualZoomPercentChanged);
}

void TestImageZoomWorkflowState::forcedTileRefreshSchedulesDecodeWithoutZoomChange()
{
    KiriView::ImageZoomWorkflowState state([]() {
        return KiriView::ImageDocumentRenderContext { 1.0, KiriView::fallbackTextureSizeMax };
    });
    state.mutate([](KiriView::ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(QSizeF(100.0, 100.0), devicePixelRatio);
    });

    const KiriView::ImageZoomWorkflowMutationResult result
        = state.mutate([](KiriView::ImageZoomState &, qreal) { }, true);

    QVERIFY(!result.changes.imageSizeChanged);
    QVERIFY(!result.changes.viewportSizeChanged);
    QVERIFY(!result.changes.zoomPercentChanged);
    QVERIFY(result.changes.scheduleVisibleTileDecode);
}

void TestImageZoomWorkflowState::clearResetsCanonicalZoomState()
{
    KiriView::ImageZoomWorkflowState state;
    state.mutate([](KiriView::ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(QSizeF(100.0, 100.0), devicePixelRatio);
        zoomState.setImageSize(QSize(50, 50), devicePixelRatio);
    });

    state.clear();

    QVERIFY(state.zoomState().imageSize().isEmpty());
    QVERIFY(state.zoomState().viewportSize().isEmpty());
    QCOMPARE(state.zoomState().zoomMode(), KiriView::ImageZoomMode::Fit);
}

QTEST_GUILESS_MAIN(TestImageZoomWorkflowState)

#include "test_imagezoomworkflowstate.moc"
