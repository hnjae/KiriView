// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportscanstate.h"

#include <QObject>
#include <QTest>

class TestImageViewportScanState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImagesStartAtInitialScanPositionByDefault();
    void pendingFinalScanStartAppliesToOneDisplayedImage();
    void cancelledPendingStartDoesNotChangeDisplayedImageStart();
};

void TestImageViewportScanState::displayedImagesStartAtInitialScanPositionByDefault()
{
    KiriView::ImageViewportScanState state;

    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.displayedImageScanStart() == KiriView::ImageViewportScanStart::Initial);
    QVERIFY(state.beginDisplayedImage() == KiriView::ImageViewportScanStart::Initial);
    QVERIFY(state.displayedImageScanStart() == KiriView::ImageViewportScanStart::Initial);
}

void TestImageViewportScanState::pendingFinalScanStartAppliesToOneDisplayedImage()
{
    KiriView::ImageViewportScanState state;

    state.requestNextDisplayedImageFinalScanStart();
    QVERIFY(state.pendingFinalScanStart());
    QVERIFY(state.beginDisplayedImage() == KiriView::ImageViewportScanStart::Final);
    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.displayedImageScanStart() == KiriView::ImageViewportScanStart::Final);

    QVERIFY(state.beginDisplayedImage() == KiriView::ImageViewportScanStart::Initial);
    QVERIFY(state.displayedImageScanStart() == KiriView::ImageViewportScanStart::Initial);
}

void TestImageViewportScanState::cancelledPendingStartDoesNotChangeDisplayedImageStart()
{
    KiriView::ImageViewportScanState state;

    state.requestNextDisplayedImageFinalScanStart();
    state.cancelPendingDisplayedImageStart();

    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.beginDisplayedImage() == KiriView::ImageViewportScanStart::Initial);
}

QTEST_GUILESS_MAIN(TestImageViewportScanState)

#include "test_imageviewportscanstate.moc"
