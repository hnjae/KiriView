// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailability.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

class TestImageActionAvailability : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageNavigationStateDrivesActionAvailability();
    void readyActionAvailabilityRejectsCompetingModes();
    void shortcutAvailabilityUsesViewerAndRuntimeGates();
    void settersNotifyOnlyWhenInputsChange();
};

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

void TestImageActionAvailability::settersNotifyOnlyWhenInputsChange()
{
    ImageActionAvailability availability;
    QSignalSpy spy(&availability, &ImageActionAvailability::availabilityChanged);

    availability.setImageReady(false);
    QCOMPARE(spy.count(), 0);

    availability.setImageReady(true);
    QCOMPARE(spy.count(), 1);

    availability.setImageReady(true);
    QCOMPARE(spy.count(), 1);

    availability.setCurrentPageNumber(1);
    QCOMPARE(spy.count(), 2);
}

QTEST_GUILESS_MAIN(TestImageActionAvailability)

#include "test_imageactionavailability.moc"
