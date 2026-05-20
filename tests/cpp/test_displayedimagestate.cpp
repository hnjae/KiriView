// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/displayedimagestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QRect>
#include <QSize>
#include <QTest>

class TestDisplayedImageState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSurfaceChangesAdvanceRevisionAndExposeImageSize();
};

void TestDisplayedImageState::imageSurfaceChangesAdvanceRevisionAndExposeImageSize()
{
    constexpr qsizetype tileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;
    KiriView::DisplayedImageState state;

    state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(1));
    QCOMPARE(state.imageSize(), QSize(2, 1));
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(state.imageSurface()->legacyFrameSurface() != nullptr);
    QVERIFY(!state.staticImage().has_value());

    state.setStaticImage(
        KiriView::TestSupport::staticTestImagePayload(
            KiriView::TestSupport::testImage(3, 2), KiriView::TestSupport::testImage(2, 2)),
        false, true, tileCacheByteBudget);

    QCOMPARE(state.revision(), quint64(2));
    QCOMPARE(state.imageSize(), QSize(3, 2));
    QVERIFY(state.isPredecodeCacheable());
    QVERIFY(state.imageSurface()->staticTileSurface() != nullptr);
    QCOMPARE(state.imageSurface()->staticTileSurface()->tileCacheByteBudget(), tileCacheByteBudget);
    QVERIFY(state.staticImage().has_value());

    QVERIFY(state.insertTile(KiriView::DecodedTile {
        KiriView::TileKey { 0, 0, 0 },
        QSize(3, 2),
        QRect(0, 0, 3, 2),
        QRect(0, 0, 3, 2),
        KiriView::TestSupport::testImage(3, 2),
    }));

    QCOMPARE(state.revision(), quint64(3));
    QCOMPARE(state.imageSize(), QSize(3, 2));

    state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(4));
    QVERIFY(!state.isPredecodeCacheable());

    QVERIFY(state.clear());

    QCOMPARE(state.revision(), quint64(5));
    QVERIFY(!state.hasImage());
    QVERIFY(state.imageSurface() == nullptr);
    QCOMPARE(state.imageSize(), QSize());
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(!state.staticImage().has_value());
    QVERIFY(!state.clear());
    QCOMPARE(state.revision(), quint64(5));
}

QTEST_GUILESS_MAIN(TestDisplayedImageState)

#include "test_displayedimagestate.moc"
