// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetiledecodestate.h"

#include <QObject>
#include <QTest>
#include <memory>

class TestImageTileDecodeState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void surfaceChangeOwnsGenerationAndClearsRequestState();
    void invalidateRejectsPendingRequests();
};

void TestImageTileDecodeState::surfaceChangeOwnsGenerationAndClearsRequestState()
{
    KiriView::ImageTileDecodeState state;
    const auto firstSurface = std::make_shared<KiriView::DisplayedImageSurface>();
    const auto secondSurface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::TileKey key { 0, 0, 0 };

    const quint64 firstGeneration = state.beginSchedule(firstSurface);
    state.start(key);
    QVERIFY(state.finish(firstGeneration, key));
    state.fail(key);
    QVERIFY(state.exclusions().contains(key));

    const quint64 repeatedGeneration = state.beginSchedule(firstSurface);
    QCOMPARE(repeatedGeneration, firstGeneration);
    QVERIFY(state.exclusions().contains(key));

    const quint64 secondGeneration = state.beginSchedule(secondSurface);
    QVERIFY(secondGeneration != firstGeneration);
    QVERIFY(!state.exclusions().contains(key));
    QVERIFY(!state.finish(firstGeneration, key));
}

void TestImageTileDecodeState::invalidateRejectsPendingRequests()
{
    KiriView::ImageTileDecodeState state;
    const auto surface = std::make_shared<KiriView::DisplayedImageSurface>();
    const KiriView::TileKey key { 0, 0, 0 };
    const quint64 generation = state.beginSchedule(surface);

    state.start(key);
    state.invalidate();

    QVERIFY(!state.finish(generation, key));
    QVERIFY(!state.exclusions().contains(key));
}

QTEST_GUILESS_MAIN(TestImageTileDecodeState)

#include "test_imagetiledecodestate.moc"
