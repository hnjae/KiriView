// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/imageactionavailability.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

class TestImageActionAvailability : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectionDerivesReadyAndModeAvailabilityFromSnapshot();
    void projectionDerivesShortcutGatesFromSnapshot();
    void scanBoundaryStateIsImageLocalAvailability();
    void readyActionAvailabilityRejectsCompetingModes();
    void shortcutAvailabilityUsesViewerAndRuntimeGates();
    void policyScopeLookupUsesApplicationScope();
    void activeImageDocumentSourceScopeLookupUsesSessionNavigationInput();
    void shortcutScopeLookupUsesProjectionFields();
    void facadeImageDocumentSourceScopeLookupUsesRuntimePolicy();
    void settersNotifyOnlyWhenInputsChange();
};

void TestImageActionAvailability::projectionDerivesReadyAndModeAvailabilityFromSnapshot()
{
    ImageActionAvailabilityInput input;
    input.imageReady = true;
    input.twoPageModeAvailable = true;
    input.rightToLeftReadingAvailable = true;

    ImageActionAvailabilityProjection projection = imageActionAvailabilityProjection(input);

    QVERIFY(projection.canUseReadyActions);
    QVERIFY(projection.canUseRotateActions);
    QVERIFY(projection.canUseTwoPageModeActions);
    QVERIFY(projection.canUseRightToLeftReadingActions);
    QVERIFY(!projection.twoPageModeActive);
    QVERIFY(!projection.rightToLeftReadingActive);

    input.twoPageModeEnabled = true;
    input.rightToLeftReadingEnabled = true;
    projection = imageActionAvailabilityProjection(input);

    QVERIFY(projection.twoPageModeActive);
    QVERIFY(projection.rightToLeftReadingActive);
    QVERIFY(!projection.canUseRotateActions);
}

void TestImageActionAvailability::projectionDerivesShortcutGatesFromSnapshot()
{
    ImageActionAvailabilityInput input;
    input.imageReady = true;
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
    QVERIFY(!projection.pannableShortcutsEnabled);
    QVERIFY(!projection.containerShortcutsEnabled);
}

void TestImageActionAvailability::scanBoundaryStateIsImageLocalAvailability()
{
    ImageActionAvailability availability;
    QSignalSpy spy(&availability, &ImageActionAvailability::availabilityChanged);

    QVERIFY(!availability.scanBackwardAtFirstImageBoundary());

    availability.setScanBackwardAtFirstImageBoundary(true);
    QVERIFY(availability.scanBackwardAtFirstImageBoundary());
    QCOMPARE(spy.count(), 1);

    availability.setScanBackwardAtFirstImageBoundary(true);
    QCOMPARE(spy.count(), 1);
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
    QVERIFY(!availability.pannableShortcutsEnabled());
    QVERIFY(!availability.containerShortcutsEnabled());
}

void TestImageActionAvailability::policyScopeLookupUsesApplicationScope()
{
    using Scope = KiriView::ApplicationActions::ImageShortcutScope;

    ImageActionAvailabilityProjection projection;
    projection.helpShortcutsEnabled = true;
    projection.readyViewerShortcutsEnabled = true;
    projection.containerShortcutsEnabled = true;

    QVERIFY(imageActionAvailabilityShortcutsEnabledForScope(projection, Scope::HelpShortcutScope));
    QVERIFY(imageActionAvailabilityShortcutsEnabledForScope(
        projection, Scope::ReadyViewerShortcutScope));
    QVERIFY(
        imageActionAvailabilityShortcutsEnabledForScope(projection, Scope::ContainerShortcutScope));
    QVERIFY(!imageActionAvailabilityShortcutsEnabledForScope(
        projection, Scope::ImageSelectionShortcutScope));
    QVERIFY(!imageActionAvailabilityShortcutsEnabledForScope(
        projection, Scope::PageViewerShortcutScope));
    QVERIFY(
        !imageActionAvailabilityShortcutsEnabledForScope(projection, Scope::ViewerShortcutScope));
}

void TestImageActionAvailability::activeImageDocumentSourceScopeLookupUsesSessionNavigationInput()
{
    using Scope = KiriView::ApplicationActions::ImageShortcutScope;

    ImageActionAvailabilityProjection projection;
    projection.helpShortcutsEnabled = true;
    projection.viewerShortcutsEnabled = true;
    projection.readyViewerShortcutsEnabled = true;

    ActiveMediaShortcutAvailabilityInput input;
    input.imageProjection = projection;
    input.activeNavigationActionsAvailable = true;

    QVERIFY(activeMediaShortcutsEnabledForScope(input, Scope::ImageSelectionShortcutScope));
    QVERIFY(activeMediaShortcutsEnabledForScope(input, Scope::PageViewerShortcutScope));

    input.activeNavigationActionsAvailable = false;
    QVERIFY(!activeMediaShortcutsEnabledForScope(input, Scope::ImageSelectionShortcutScope));
    QVERIFY(!activeMediaShortcutsEnabledForScope(input, Scope::PageViewerShortcutScope));

    input.activeNavigationActionsAvailable = true;
    input.videoMode = true;
    input.videoFileDeletionInProgress = true;
    QVERIFY(activeMediaShortcutsEnabledForScope(input, Scope::PageShortcutScope));
    QVERIFY(!activeMediaShortcutsEnabledForScope(input, Scope::ReadyViewerShortcutScope));

    QVERIFY(!activeMediaShortcutsEnabledForScope(input, static_cast<Scope>(999)));
}

void TestImageActionAvailability::shortcutScopeLookupUsesProjectionFields()
{
    ImageActionAvailability availability;
    availability.setImageReady(true);
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
    QVERIFY(!availability.shortcutsEnabledForScope(
        ImageActionAvailability::ImageSelectionShortcutScope));
    QVERIFY(!availability.shortcutsEnabledForScope(
        ImageActionAvailability::ImageSelectionViewerShortcutScope));
    QVERIFY(!availability.shortcutsEnabledForScope(ImageActionAvailability::PageShortcutScope));
    QVERIFY(
        !availability.shortcutsEnabledForScope(ImageActionAvailability::PageViewerShortcutScope));
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

void TestImageActionAvailability::facadeImageDocumentSourceScopeLookupUsesRuntimePolicy()
{
    ImageActionAvailability availability;
    availability.setImageReady(true);

    QVERIFY(availability.mediaShortcutsEnabledForScope(
        ImageActionAvailability::ImageSelectionShortcutScope, false, true, false));
    QVERIFY(availability.mediaShortcutsEnabledForScope(
        ImageActionAvailability::ImageSelectionViewerShortcutScope, true, true, false));
    QVERIFY(!availability.mediaShortcutsEnabledForScope(
        ImageActionAvailability::ImageSelectionViewerShortcutScope, true, false, false));
    QVERIFY(availability.mediaShortcutsEnabledForScope(
        ImageActionAvailability::ReadyViewerShortcutScope, true, true, false));
    QVERIFY(!availability.mediaShortcutsEnabledForScope(
        ImageActionAvailability::ReadyViewerShortcutScope, true, true, true));
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

    availability.setScanBackwardAtFirstImageBoundary(true);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(availability.availabilityRevision(), 2);
}

QTEST_GUILESS_MAIN(TestImageActionAvailability)

#include "test_imageactionavailability.moc"
