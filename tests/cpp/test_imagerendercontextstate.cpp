// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerendercontextstate.h"

#include "rendering/imagerendering.h"

#include <QObject>
#include <QTest>
#include <limits>

class TestImageRenderContextState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsNormalizedInitialContext();
    void refreshReportsPreviousAndCurrentContext();
    void missingProviderUsesSafeDefaults();
};

void TestImageRenderContextState::ownsNormalizedInitialContext()
{
    const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
    KiriView::ImageRenderContextState state(
        [nan]() { return KiriView::ImageDocumentRenderContext { nan, 0 }; });

    QCOMPARE(state.current().devicePixelRatio, 1.0);
    QCOMPARE(state.current().maximumTextureSize, KiriView::fallbackTextureSizeMax);
    QCOMPARE(state.devicePixelRatio(), 1.0);
}

void TestImageRenderContextState::refreshReportsPreviousAndCurrentContext()
{
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = 4096;
    KiriView::ImageRenderContextState state([&]() {
        return KiriView::ImageDocumentRenderContext { devicePixelRatio, maximumTextureSize };
    });

    devicePixelRatio = 2.0;
    maximumTextureSize = 8192;
    const KiriView::ImageRenderContextChange change = state.refresh();

    QCOMPARE(change.previous.devicePixelRatio, 1.0);
    QCOMPARE(change.previous.maximumTextureSize, 4096);
    QCOMPARE(change.current.devicePixelRatio, 2.0);
    QCOMPARE(change.current.maximumTextureSize, 8192);
    QCOMPARE(state.current().devicePixelRatio, 2.0);
    QCOMPARE(state.current().maximumTextureSize, 8192);
}

void TestImageRenderContextState::missingProviderUsesSafeDefaults()
{
    KiriView::ImageRenderContextState state;

    QCOMPARE(state.current().devicePixelRatio, 1.0);
    QCOMPARE(state.current().maximumTextureSize, KiriView::fallbackTextureSizeMax);
}

QTEST_GUILESS_MAIN(TestImageRenderContextState)

#include "test_imagerendercontextstate.moc"
