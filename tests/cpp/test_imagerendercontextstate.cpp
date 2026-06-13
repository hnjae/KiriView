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
    void refreshPreservesProviderGeneration();
    void missingProviderUsesSafeDefaults();
};

void TestImageRenderContextState::ownsNormalizedInitialContext()
{
    const qreal nan = std::numeric_limits<qreal>::quiet_NaN();
    kiriview::ImageRenderContextState state(
        [nan]() { return kiriview::ImageDocumentRenderContext { nan, 0, 9 }; });

    QCOMPARE(state.current().devicePixelRatio, 1.0);
    QCOMPARE(state.current().maximumTextureSize, kiriview::fallbackTextureSizeMax);
    QCOMPARE(state.current().generation, quint64(9));
    QCOMPARE(state.devicePixelRatio(), 1.0);
}

void TestImageRenderContextState::refreshReportsPreviousAndCurrentContext()
{
    qreal devicePixelRatio = 1.0;
    int maximumTextureSize = 4096;
    kiriview::ImageRenderContextState state([&]() {
        return kiriview::ImageDocumentRenderContext { devicePixelRatio, maximumTextureSize, 1 };
    });

    devicePixelRatio = 2.0;
    maximumTextureSize = 8192;
    const kiriview::ImageRenderContextChange change = state.refresh();

    QCOMPARE(change.previous.devicePixelRatio, 1.0);
    QCOMPARE(change.previous.maximumTextureSize, 4096);
    QCOMPARE(change.previous.generation, quint64(1));
    QCOMPARE(change.current.devicePixelRatio, 2.0);
    QCOMPARE(change.current.maximumTextureSize, 8192);
    QCOMPARE(change.current.generation, quint64(1));
    QCOMPARE(state.current().devicePixelRatio, 2.0);
    QCOMPARE(state.current().maximumTextureSize, 8192);
}

void TestImageRenderContextState::refreshPreservesProviderGeneration()
{
    quint64 generation = 5;
    kiriview::ImageRenderContextState state([&generation]() {
        return kiriview::ImageDocumentRenderContext {
            1.0,
            kiriview::fallbackTextureSizeMax,
            generation,
        };
    });

    generation = 6;
    const kiriview::ImageRenderContextChange change = state.refresh();

    QCOMPARE(change.previous.generation, quint64(5));
    QCOMPARE(change.current.generation, quint64(6));
    QCOMPARE(state.current().generation, quint64(6));
}

void TestImageRenderContextState::missingProviderUsesSafeDefaults()
{
    kiriview::ImageRenderContextState state;

    QCOMPARE(state.current().devicePixelRatio, 1.0);
    QCOMPARE(state.current().maximumTextureSize, kiriview::fallbackTextureSizeMax);
    QCOMPARE(state.current().generation, quint64(0));
}

QTEST_GUILESS_MAIN(TestImageRenderContextState)

#include "test_imagerendercontextstate.moc"
