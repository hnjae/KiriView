// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayedimagestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QRect>
#include <QSize>
#include <QTest>
#include <vector>

class TestDisplayedImageState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSurfaceChangesAdvanceRevisionAndNotify();
};

void TestDisplayedImageState::imageSurfaceChangesAdvanceRevisionAndNotify()
{
    constexpr qsizetype tileCacheByteBudget = KiriView::imageFullDecodeFallbackByteLimit;
    std::vector<QSize> changedSizes;
    KiriView::DisplayedImageState state(
        this, [&changedSizes](const QSize &size) { changedSizes.push_back(size); }, {});

    state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(1));
    QCOMPARE(state.imageSize(), QSize(2, 1));
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(state.imageSurface()->legacyFrameSurface() != nullptr);
    QVERIFY(!state.staticImage().has_value());
    QCOMPARE(changedSizes.back(), QSize(2, 1));

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
    QCOMPARE(changedSizes.back(), QSize(3, 2));

    QVERIFY(state.insertTile(KiriView::DecodedTile {
        KiriView::TileKey { 0, 0, 0 },
        QSize(3, 2),
        QRect(0, 0, 3, 2),
        QRect(0, 0, 3, 2),
        KiriView::TestSupport::testImage(3, 2),
    }));

    QCOMPARE(state.revision(), quint64(3));
    QCOMPARE(changedSizes.back(), QSize(3, 2));

    state.setImage(KiriView::TestSupport::testImage(2, 1), false);

    QCOMPARE(state.revision(), quint64(4));
    QVERIFY(!state.isPredecodeCacheable());

    state.clear();

    QCOMPARE(state.revision(), quint64(5));
    QVERIFY(!state.hasImage());
    QVERIFY(state.imageSurface() == nullptr);
    QCOMPARE(state.imageSize(), QSize());
    QVERIFY(!state.isPredecodeCacheable());
    QVERIFY(!state.staticImage().has_value());
    QCOMPARE(changedSizes.back(), QSize());
}

QTEST_GUILESS_MAIN(TestDisplayedImageState)

#include "test_displayedimagestate.moc"
