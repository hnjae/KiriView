// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imageactionavailabilityconversion.h"

#include <QObject>
#include <QTest>

class TestImageActionAvailabilityConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageAvailabilityInputMapsPlainFields();
    void imageAvailabilityProjectionMapsPlainFieldsToRust();
    void projectionMapsPlainFields();
    void shortcutScopeMapsApplicationScopeValues();
    void videoAvailabilityInputMapsPlainFields();
};

void TestImageActionAvailabilityConversion::imageAvailabilityInputMapsPlainFields()
{
    ImageActionAvailabilityInput input;
    input.imageReady = true;
    input.fileDeletionInProgress = true;
    input.helpDialogOpen = true;
    input.textInputFocused = true;
    input.imagePannable = true;
    input.containerNavigationAvailable = true;
    input.twoPageModeEnabled = true;
    input.twoPageModeAvailable = true;
    input.rightToLeftReadingEnabled = true;
    input.rightToLeftReadingAvailable = true;

    const kiriview::RustImageActionAvailabilityInput converted
        = kiriview::Bridge::rustImageActionAvailabilityInput(input);

    QVERIFY(converted.image_ready);
    QVERIFY(converted.file_deletion_in_progress);
    QVERIFY(converted.help_dialog_open);
    QVERIFY(converted.text_input_focused);
    QVERIFY(converted.image_pannable);
    QVERIFY(converted.container_navigation_available);
    QVERIFY(converted.two_page_mode_enabled);
    QVERIFY(converted.two_page_mode_available);
    QVERIFY(converted.right_to_left_reading_enabled);
    QVERIFY(converted.right_to_left_reading_available);
}

void TestImageActionAvailabilityConversion::imageAvailabilityProjectionMapsPlainFieldsToRust()
{
    ImageActionAvailabilityProjection projection;
    projection.canUseReadyActions = true;
    projection.canUseRotateActions = true;
    projection.canUseTwoPageModeActions = true;
    projection.canUseRightToLeftReadingActions = true;
    projection.rightToLeftReadingActive = true;
    projection.twoPageModeActive = true;
    projection.helpShortcutsEnabled = true;
    projection.viewerShortcutsEnabled = true;
    projection.readyShortcutsEnabled = true;
    projection.readyViewerShortcutsEnabled = true;
    projection.twoPageViewerShortcutsEnabled = true;
    projection.rightToLeftReadingShortcutsEnabled = true;
    projection.rightToLeftReadingViewerShortcutsEnabled = true;
    projection.rotateShortcutsEnabled = true;
    projection.rotateViewerShortcutsEnabled = true;
    projection.pannableShortcutsEnabled = true;
    projection.pannableViewerShortcutsEnabled = true;
    projection.containerShortcutsEnabled = true;
    projection.containerViewerShortcutsEnabled = true;

    const kiriview::RustImageActionAvailabilityProjection converted
        = kiriview::Bridge::rustImageActionAvailabilityProjection(projection);

    QVERIFY(converted.can_use_ready_actions);
    QVERIFY(converted.can_use_rotate_actions);
    QVERIFY(converted.can_use_two_page_mode_actions);
    QVERIFY(converted.can_use_right_to_left_reading_actions);
    QVERIFY(converted.right_to_left_reading_active);
    QVERIFY(converted.two_page_mode_active);
    QVERIFY(converted.help_shortcuts_enabled);
    QVERIFY(converted.viewer_shortcuts_enabled);
    QVERIFY(converted.ready_shortcuts_enabled);
    QVERIFY(converted.ready_viewer_shortcuts_enabled);
    QVERIFY(converted.two_page_viewer_shortcuts_enabled);
    QVERIFY(converted.right_to_left_reading_shortcuts_enabled);
    QVERIFY(converted.right_to_left_reading_viewer_shortcuts_enabled);
    QVERIFY(converted.rotate_shortcuts_enabled);
    QVERIFY(converted.rotate_viewer_shortcuts_enabled);
    QVERIFY(converted.pannable_shortcuts_enabled);
    QVERIFY(converted.pannable_viewer_shortcuts_enabled);
    QVERIFY(converted.container_shortcuts_enabled);
    QVERIFY(converted.container_viewer_shortcuts_enabled);
}

void TestImageActionAvailabilityConversion::projectionMapsPlainFields()
{
    kiriview::RustImageActionAvailabilityProjection projection {};
    projection.can_use_ready_actions = true;
    projection.can_use_rotate_actions = true;
    projection.can_use_two_page_mode_actions = true;
    projection.can_use_right_to_left_reading_actions = true;
    projection.right_to_left_reading_active = true;
    projection.two_page_mode_active = true;
    projection.help_shortcuts_enabled = true;
    projection.viewer_shortcuts_enabled = true;
    projection.ready_shortcuts_enabled = true;
    projection.ready_viewer_shortcuts_enabled = true;
    projection.two_page_viewer_shortcuts_enabled = true;
    projection.right_to_left_reading_shortcuts_enabled = true;
    projection.right_to_left_reading_viewer_shortcuts_enabled = true;
    projection.rotate_shortcuts_enabled = true;
    projection.rotate_viewer_shortcuts_enabled = true;
    projection.pannable_shortcuts_enabled = true;
    projection.pannable_viewer_shortcuts_enabled = true;
    projection.container_shortcuts_enabled = true;
    projection.container_viewer_shortcuts_enabled = true;

    const ImageActionAvailabilityProjection converted
        = kiriview::Bridge::imageActionAvailabilityProjectionFromRust(projection);

    QVERIFY(converted.canUseReadyActions);
    QVERIFY(converted.canUseRotateActions);
    QVERIFY(converted.canUseTwoPageModeActions);
    QVERIFY(converted.canUseRightToLeftReadingActions);
    QVERIFY(converted.rightToLeftReadingActive);
    QVERIFY(converted.twoPageModeActive);
    QVERIFY(converted.helpShortcutsEnabled);
    QVERIFY(converted.viewerShortcutsEnabled);
    QVERIFY(converted.readyShortcutsEnabled);
    QVERIFY(converted.readyViewerShortcutsEnabled);
    QVERIFY(converted.twoPageViewerShortcutsEnabled);
    QVERIFY(converted.rightToLeftReadingShortcutsEnabled);
    QVERIFY(converted.rightToLeftReadingViewerShortcutsEnabled);
    QVERIFY(converted.rotateShortcutsEnabled);
    QVERIFY(converted.rotateViewerShortcutsEnabled);
    QVERIFY(converted.pannableShortcutsEnabled);
    QVERIFY(converted.pannableViewerShortcutsEnabled);
    QVERIFY(converted.containerShortcutsEnabled);
    QVERIFY(converted.containerViewerShortcutsEnabled);
}

void TestImageActionAvailabilityConversion::shortcutScopeMapsApplicationScopeValues()
{
    using Scope = kiriview::ApplicationActions::ImageShortcutScope;

    QVERIFY(kiriview::Bridge::rustImageShortcutScope(Scope::HelpShortcutScope)
        == kiriview::RustImageShortcutScope::HelpShortcutScope);
    QVERIFY(kiriview::Bridge::rustImageShortcutScope(Scope::ReadyViewerShortcutScope)
        == kiriview::RustImageShortcutScope::ReadyViewerShortcutScope);
    QVERIFY(kiriview::Bridge::rustImageShortcutScope(Scope::RightToLeftReadingViewerShortcutScope)
        == kiriview::RustImageShortcutScope::RightToLeftReadingViewerShortcutScope);
    QVERIFY(kiriview::Bridge::rustImageShortcutScope(Scope::ContainerViewerShortcutScope)
        == kiriview::RustImageShortcutScope::ContainerViewerShortcutScope);
    QVERIFY(kiriview::Bridge::rustImageShortcutScope(Scope::MediaStartEndViewerShortcutScope)
        == kiriview::RustImageShortcutScope::MediaStartEndViewerShortcutScope);
}

void TestImageActionAvailabilityConversion::videoAvailabilityInputMapsPlainFields()
{
    kiriview::ApplicationActions::VideoShortcutAvailabilityInput input;
    input.helpShortcutsEnabled = true;
    input.viewerShortcutsEnabled = true;
    input.fileDeletionInProgress = true;
    input.directMediaNavigationActive = true;

    const kiriview::RustVideoShortcutAvailabilityInput converted
        = kiriview::Bridge::rustVideoShortcutAvailabilityInput(input);

    QVERIFY(converted.help_shortcuts_enabled);
    QVERIFY(converted.viewer_shortcuts_enabled);
    QVERIFY(converted.file_deletion_in_progress);
    QVERIFY(converted.media_navigation_active);
}

QTEST_GUILESS_MAIN(TestImageActionAvailabilityConversion)

#include "test_imageactionavailabilityconversion.moc"
