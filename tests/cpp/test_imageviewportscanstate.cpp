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
    kiriview::ImageViewportScanState state;

    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.displayedImageScanStart() == kiriview::ImageViewportScanStart::Initial);
    QVERIFY(state.beginDisplayedImage() == kiriview::ImageViewportScanStart::Initial);
    QVERIFY(state.displayedImageScanStart() == kiriview::ImageViewportScanStart::Initial);
}

void TestImageViewportScanState::pendingFinalScanStartAppliesToOneDisplayedImage()
{
    kiriview::ImageViewportScanState state;

    state.requestNextDisplayedImageFinalScanStart();
    QVERIFY(state.pendingFinalScanStart());
    QVERIFY(state.beginDisplayedImage() == kiriview::ImageViewportScanStart::Final);
    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.displayedImageScanStart() == kiriview::ImageViewportScanStart::Final);

    QVERIFY(state.beginDisplayedImage() == kiriview::ImageViewportScanStart::Initial);
    QVERIFY(state.displayedImageScanStart() == kiriview::ImageViewportScanStart::Initial);
}

void TestImageViewportScanState::cancelledPendingStartDoesNotChangeDisplayedImageStart()
{
    kiriview::ImageViewportScanState state;

    state.requestNextDisplayedImageFinalScanStart();
    state.cancelPendingDisplayedImageStart();

    QVERIFY(!state.pendingFinalScanStart());
    QVERIFY(state.beginDisplayedImage() == kiriview::ImageViewportScanStart::Initial);
}

QTEST_GUILESS_MAIN(TestImageViewportScanState)

#include "test_imageviewportscanstate.moc"
