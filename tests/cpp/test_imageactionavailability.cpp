// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/imageactionavailability.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

class TestImageActionAvailability : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectionDerivesNavigationAndModeAvailabilityFromSnapshot();
    void projectionDerivesShortcutGatesFromSnapshot();
    void imageNavigationStateDrivesActionAvailability();
    void readyActionAvailabilityRejectsCompetingModes();
    void shortcutAvailabilityUsesViewerAndRuntimeGates();
    void shortcutScopeLookupUsesProjectionFields();
    void settersNotifyOnlyWhenInputsChange();
};

void TestImageActionAvailability::projectionDerivesNavigationAndModeAvailabilityFromSnapshot()
{
    ImageActionAvailabilityInput input;
    input.imageReady = true;
    input.imageCount = 5;
    input.currentPageNumber = 1;
    input.currentLastPageNumber = 2;
    input.twoPageModeAvailable = true;
    input.rightToLeftReadingAvailable = true;

    ImageActionAvailabilityProjection projection = imageActionAvailabilityProjection(input);

    QVERIFY(projection.canUsePageActions);
    QVERIFY(!projection.canOpenPreviousImage);
    QVERIFY(projection.canOpenNextImage);
    QVERIFY(projection.atKnownFirstImage);
    QVERIFY(!projection.atKnownLastImage);
    QVERIFY(projection.canUseReadyActions);
    QVERIFY(projection.canUseRotateActions);
    QVERIFY(projection.canUseTwoPageModeActions);
    QVERIFY(projection.canUseRightToLeftReadingActions);
    QVERIFY(!projection.twoPageModeActive);
    QVERIFY(!projection.rightToLeftReadingActive);

    input.currentPageNumber = 5;
    input.currentLastPageNumber = 5;
    input.twoPageModeEnabled = true;
    input.rightToLeftReadingEnabled = true;
    projection = imageActionAvailabilityProjection(input);

    QVERIFY(projection.canOpenPreviousImage);
    QVERIFY(!projection.canOpenNextImage);
    QVERIFY(!projection.atKnownFirstImage);
    QVERIFY(projection.atKnownLastImage);
    QVERIFY(projection.twoPageModeActive);
    QVERIFY(projection.rightToLeftReadingActive);
    QVERIFY(!projection.canUseRotateActions);
}

void TestImageActionAvailability::projectionDerivesShortcutGatesFromSnapshot()
{
    ImageActionAvailabilityInput input;
    input.imageReady = true;
    input.imageCount = 3;
    input.currentPageNumber = 2;
    input.imagePannable = true;
    input.containerNavigationAvailable = true;
    input.twoPageModeEnabled = true;
    input.twoPageModeAvailable = true;
    input.rightToLeftReadingAvailable = true;

    ImageActionAvailabilityProjection projection = imageActionAvailabilityProjection(input);

    QVERIFY(projection.helpShortcutsEnabled);
    QVERIFY(projection.viewerShortcutsEnabled);
    QVERIFY(projection.readyShortcutsEnabled);
    QVERIFY(projection.readyViewerShortcutsEnabled);
    QVERIFY(projection.imageSelectionShortcutsEnabled);
    QVERIFY(projection.imageSelectionViewerShortcutsEnabled);
    QVERIFY(projection.pageShortcutsEnabled);
    QVERIFY(projection.pageViewerShortcutsEnabled);
    QVERIFY(projection.twoPageViewerShortcutsEnabled);
    QVERIFY(projection.rightToLeftReadingShortcutsEnabled);
    QVERIFY(projection.rightToLeftReadingViewerShortcutsEnabled);
    QVERIFY(projection.pannableShortcutsEnabled);
    QVERIFY(projection.pannableViewerShortcutsEnabled);
    QVERIFY(projection.containerShortcutsEnabled);
    QVERIFY(projection.containerViewerShortcutsEnabled);
    QVERIFY(!projection.rotateShortcutsEnabled);
    QVERIFY(!projection.rotateViewerShortcutsEnabled);

    input.textInputFocused = true;
    projection = imageActionAvailabilityProjection(input);

    QVERIFY(!projection.viewerShortcutsEnabled);
    QVERIFY(projection.readyShortcutsEnabled);
    QVERIFY(!projection.readyViewerShortcutsEnabled);
    QVERIFY(!projection.pannableViewerShortcutsEnabled);
    QVERIFY(!projection.containerViewerShortcutsEnabled);

    input.fileDeletionInProgress = true;
    projection = imageActionAvailabilityProjection(input);

    QVERIFY(!projection.readyShortcutsEnabled);
    QVERIFY(!projection.imageSelectionShortcutsEnabled);
    QVERIFY(!projection.pannableShortcutsEnabled);
    QVERIFY(!projection.containerShortcutsEnabled);
}

void TestImageActionAvailability::imageNavigationStateDrivesActionAvailability()
{
    ImageActionAvailability availability;
    availability.setImageCount(5);
    availability.setCurrentPageNumber(1);
    availability.setCurrentLastPageNumber(1);

    QVERIFY(availability.canUsePageActions());
    QVERIFY(!availability.canOpenPreviousImage());
    QVERIFY(availability.canOpenNextImage());
    QVERIFY(availability.atKnownFirstImage());
    QVERIFY(!availability.atKnownLastImage());

    availability.setCurrentPageNumber(5);
    availability.setCurrentLastPageNumber(5);

    QVERIFY(availability.canOpenPreviousImage());
    QVERIFY(!availability.canOpenNextImage());
    QVERIFY(!availability.atKnownFirstImage());
    QVERIFY(availability.atKnownLastImage());

    availability.setFileDeletionInProgress(true);
    QVERIFY(!availability.canUsePageActions());
}

void TestImageActionAvailability::readyActionAvailabilityRejectsCompetingModes()
{
    ImageActionAvailability availability;
    availability.setImageReady(true);
    availability.setTwoPageModeAvailable(true);
    availability.setRightToLeftReadingAvailable(true);

    QVERIFY(availability.canUseReadyActions());
    QVERIFY(availability.canUseRotateActions());
    QVERIFY(availability.canUseTwoPageModeActions());
    QVERIFY(availability.canUseRightToLeftReadingActions());
    QVERIFY(!availability.twoPageModeActive());

    availability.setTwoPageModeEnabled(true);
    QVERIFY(availability.twoPageModeActive());
    QVERIFY(!availability.canUseRotateActions());

    availability.setRightToLeftReadingEnabled(true);
    QVERIFY(availability.rightToLeftReadingActive());

    availability.setHelpDialogOpen(true);
    QVERIFY(!availability.canUseReadyActions());
    QVERIFY(!availability.canUseTwoPageModeActions());
    QVERIFY(!availability.canUseRightToLeftReadingActions());
}

void TestImageActionAvailability::shortcutAvailabilityUsesViewerAndRuntimeGates()
{
    ImageActionAvailability availability;
    availability.setImageReady(true);
    availability.setImageCount(5);
    availability.setCurrentPageNumber(2);
    availability.setImagePannable(true);
    availability.setImageHorizontallyPannable(true);
    availability.setContainerNavigationAvailable(true);
    availability.setTwoPageModeAvailable(true);
    availability.setTwoPageModeEnabled(true);
    availability.setRightToLeftReadingAvailable(true);

    QVERIFY(availability.helpShortcutsEnabled());
    QVERIFY(availability.viewerShortcutsEnabled());
    QVERIFY(availability.readyShortcutsEnabled());
    QVERIFY(availability.readyViewerShortcutsEnabled());
    QVERIFY(availability.imageSelectionShortcutsEnabled());
    QVERIFY(availability.imageSelectionViewerShortcutsEnabled());
    QVERIFY(availability.pageShortcutsEnabled());
    QVERIFY(availability.pageViewerShortcutsEnabled());
    QVERIFY(availability.twoPageViewerShortcutsEnabled());
    QVERIFY(availability.rightToLeftReadingShortcutsEnabled());
    QVERIFY(availability.rightToLeftReadingViewerShortcutsEnabled());
    QVERIFY(availability.pannableShortcutsEnabled());
    QVERIFY(availability.pannableViewerShortcutsEnabled());
    QVERIFY(availability.containerShortcutsEnabled());
    QVERIFY(availability.containerViewerShortcutsEnabled());
    QVERIFY(!availability.rotateShortcutsEnabled());
    QVERIFY(!availability.rotateViewerShortcutsEnabled());
    QVERIFY(availability.imageHorizontallyPannable());

    availability.setTextInputFocused(true);
    QVERIFY(!availability.viewerShortcutsEnabled());
    QVERIFY(availability.readyShortcutsEnabled());
    QVERIFY(!availability.readyViewerShortcutsEnabled());
    QVERIFY(!availability.pannableViewerShortcutsEnabled());
    QVERIFY(!availability.containerViewerShortcutsEnabled());

    availability.setFileDeletionInProgress(true);
    QVERIFY(!availability.readyShortcutsEnabled());
    QVERIFY(!availability.imageSelectionShortcutsEnabled());
    QVERIFY(!availability.pannableShortcutsEnabled());
    QVERIFY(!availability.containerShortcutsEnabled());
}

void TestImageActionAvailability::shortcutScopeLookupUsesProjectionFields()
{
    ImageActionAvailability availability;
    availability.setImageReady(true);
    availability.setImageCount(5);
    availability.setCurrentPageNumber(2);
    availability.setImagePannable(true);
    availability.setContainerNavigationAvailable(true);
    availability.setTwoPageModeAvailable(true);
    availability.setRightToLeftReadingAvailable(true);

    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::HelpShortcutScope),
        availability.helpShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::ViewerShortcutScope),
        availability.viewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::ReadyShortcutScope),
        availability.readyShortcutsEnabled());
    QCOMPARE(
        availability.shortcutsEnabledForScope(ImageActionAvailability::ReadyViewerShortcutScope),
        availability.readyViewerShortcutsEnabled());
    QCOMPARE(
        availability.shortcutsEnabledForScope(ImageActionAvailability::ImageSelectionShortcutScope),
        availability.imageSelectionShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(
                 ImageActionAvailability::ImageSelectionViewerShortcutScope),
        availability.imageSelectionViewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::PageShortcutScope),
        availability.pageShortcutsEnabled());
    QCOMPARE(
        availability.shortcutsEnabledForScope(ImageActionAvailability::PageViewerShortcutScope),
        availability.pageViewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(
                 ImageActionAvailability::RightToLeftReadingShortcutScope),
        availability.rightToLeftReadingShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(
                 ImageActionAvailability::RightToLeftReadingViewerShortcutScope),
        availability.rightToLeftReadingViewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::RotateShortcutScope),
        availability.rotateShortcutsEnabled());
    QCOMPARE(
        availability.shortcutsEnabledForScope(ImageActionAvailability::RotateViewerShortcutScope),
        availability.rotateViewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::PannableShortcutScope),
        availability.pannableShortcutsEnabled());
    QCOMPARE(
        availability.shortcutsEnabledForScope(ImageActionAvailability::PannableViewerShortcutScope),
        availability.pannableViewerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(ImageActionAvailability::ContainerShortcutScope),
        availability.containerShortcutsEnabled());
    QCOMPARE(availability.shortcutsEnabledForScope(
                 ImageActionAvailability::ContainerViewerShortcutScope),
        availability.containerViewerShortcutsEnabled());
    QVERIFY(!availability.shortcutsEnabledForScope(
        static_cast<ImageActionAvailability::ShortcutScope>(999)));
}

void TestImageActionAvailability::settersNotifyOnlyWhenInputsChange()
{
    ImageActionAvailability availability;
    QSignalSpy spy(&availability, &ImageActionAvailability::availabilityChanged);
    QCOMPARE(availability.availabilityRevision(), 0);

    availability.setImageReady(false);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(availability.availabilityRevision(), 0);

    availability.setImageReady(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(availability.availabilityRevision(), 1);

    availability.setImageReady(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(availability.availabilityRevision(), 1);

    availability.setCurrentPageNumber(1);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(availability.availabilityRevision(), 2);
}

QTEST_GUILESS_MAIN(TestImageActionAvailability)

#include "test_imageactionavailability.moc"
