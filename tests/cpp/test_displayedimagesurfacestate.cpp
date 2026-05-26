// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/displayedimagesurfacestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QRect>
#include <QSize>
#include <QTest>
#include <optional>

class TestDisplayedImageSurfaceState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSurfaceChangesAdvanceRevisionAndExposeImageSize();
    void nonStaticSurfacesRejectTilesWithoutAdvancingRevision();
};

void TestDisplayedImageSurfaceState::imageSurfaceChangesAdvanceRevisionAndExposeImageSize()
{
    constexpr qsizetype tileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;
    KiriView::DisplayedImageSurfaceState state;

    KiriView::DisplayedImageSurfaceStateChange change
        = state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(1));
    QCOMPARE(change.revision, quint64(1));
    QCOMPARE(change.imageSize, QSize(2, 1));
    QCOMPARE(state.imageSize(), QSize(2, 1));
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(state.imageSurface()->legacyFrameSurface() != nullptr);
    QVERIFY(!state.staticImage().has_value());

    change = state.setStaticImage(
        KiriView::TestSupport::staticTestImagePayload(
            KiriView::TestSupport::testImage(3, 2), KiriView::TestSupport::testImage(2, 2)),
        false, true, tileCacheByteBudget);

    QCOMPARE(state.revision(), quint64(2));
    QCOMPARE(change.revision, quint64(2));
    QCOMPARE(change.imageSize, QSize(3, 2));
    QCOMPARE(state.imageSize(), QSize(3, 2));
    QVERIFY(state.isPredecodeCacheable());
    QVERIFY(state.imageSurface()->staticTileSurface() != nullptr);
    QCOMPARE(state.imageSurface()->staticTileSurface()->tileCacheByteBudget(), tileCacheByteBudget);
    QVERIFY(state.staticImage().has_value());

    std::optional<KiriView::DisplayedImageSurfaceStateChange> optionalChange
        = state.insertTile(KiriView::DecodedTile {
            KiriView::TileKey { 0, 0, 0 },
            QSize(3, 2),
            QRect(0, 0, 3, 2),
            QRect(0, 0, 3, 2),
            KiriView::TestSupport::testImage(3, 2),
        });

    QVERIFY(optionalChange.has_value());
    QCOMPARE(optionalChange->revision, quint64(3));
    QCOMPARE(optionalChange->imageSize, QSize(3, 2));
    QCOMPARE(state.revision(), quint64(3));
    QCOMPARE(state.imageSize(), QSize(3, 2));

    change = state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(4));
    QCOMPARE(change.revision, quint64(4));
    QCOMPARE(change.imageSize, QSize(2, 1));
    QVERIFY(!state.isPredecodeCacheable());

    optionalChange = state.clear();

    QVERIFY(optionalChange.has_value());
    QCOMPARE(optionalChange->revision, quint64(5));
    QCOMPARE(optionalChange->imageSize, QSize());
    QCOMPARE(state.revision(), quint64(5));
    QVERIFY(!state.hasImage());
    QVERIFY(state.imageSurface() == nullptr);
    QCOMPARE(state.imageSize(), QSize());
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(!state.staticImage().has_value());
    QVERIFY(!state.clear().has_value());
    QCOMPARE(state.revision(), quint64(5));
}

void TestDisplayedImageSurfaceState::nonStaticSurfacesRejectTilesWithoutAdvancingRevision()
{
    KiriView::DisplayedImageSurfaceState state;

    QVERIFY(!state.insertTile(KiriView::DecodedTile {}).has_value());
    QCOMPARE(state.revision(), quint64(0));

    state.setImage(KiriView::TestSupport::testImage(2, 1), false);
    QVERIFY(state.imageSurface()->legacyFrameSurface() != nullptr);

    QVERIFY(!state
            .insertTile(KiriView::DecodedTile {
                KiriView::TileKey { 0, 0, 0 },
                QSize(2, 1),
                QRect(0, 0, 2, 1),
                QRect(0, 0, 2, 1),
                KiriView::TestSupport::testImage(2, 1),
            })
            .has_value());
    QCOMPARE(state.revision(), quint64(1));
}

QTEST_GUILESS_MAIN(TestDisplayedImageSurfaceState)

#include "test_displayedimagesurfacestate.moc"
