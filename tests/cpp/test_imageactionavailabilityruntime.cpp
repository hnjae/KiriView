// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/imageactionavailabilityruntime.h"

#include <QObject>
#include <QTest>

class TestImageActionAvailabilityRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultProjectionStartsDisabledAndUnrevised();
    void settersRecomputeProjectionAndNotifyOnce();
    void unchangedInputsDoNotNotifyOrRevise();
    void shortcutScopeLookupUsesProjectionFields();
    void mediaShortcutScopeLookupCombinesSessionAndVideoInputs();
};

void TestImageActionAvailabilityRuntime::defaultProjectionStartsDisabledAndUnrevised()
{
    KiriView::ApplicationActions::ImageActionAvailabilityRuntime runtime;

    QVERIFY(!runtime.imageReady());
    QVERIFY(!runtime.canUseReadyActions());
    QVERIFY(runtime.helpShortcutsEnabled());
    QVERIFY(!runtime.readyShortcutsEnabled());
    QCOMPARE(runtime.availabilityRevision(), 0);
}

void TestImageActionAvailabilityRuntime::settersRecomputeProjectionAndNotifyOnce()
{
    int changeCount = 0;
    KiriView::ApplicationActions::ImageActionAvailabilityRuntime runtime(
        [&changeCount]() { ++changeCount; });

    runtime.setImageReady(true);

    QVERIFY(runtime.imageReady());
    QVERIFY(runtime.canUseReadyActions());
    QVERIFY(runtime.canUseRotateActions());
    QVERIFY(runtime.readyShortcutsEnabled());
    QCOMPARE(runtime.availabilityRevision(), 1);
    QCOMPARE(changeCount, 1);
}

void TestImageActionAvailabilityRuntime::unchangedInputsDoNotNotifyOrRevise()
{
    int changeCount = 0;
    KiriView::ApplicationActions::ImageActionAvailabilityRuntime runtime(
        [&changeCount]() { ++changeCount; });

    runtime.setImageReady(false);
    QCOMPARE(runtime.availabilityRevision(), 0);
    QCOMPARE(changeCount, 0);

    runtime.setImageReady(true);
    runtime.setImageReady(true);
    QCOMPARE(runtime.availabilityRevision(), 1);
    QCOMPARE(changeCount, 1);
}

void TestImageActionAvailabilityRuntime::shortcutScopeLookupUsesProjectionFields()
{
    using Scope = KiriView::ApplicationActions::ImageShortcutScope;

    KiriView::ApplicationActions::ImageActionAvailabilityRuntime runtime;
    runtime.setImageReady(true);
    runtime.setImagePannable(true);
    runtime.setContainerNavigationAvailable(true);
    runtime.setTwoPageModeAvailable(true);
    runtime.setRightToLeftReadingAvailable(true);

    QCOMPARE(
        runtime.shortcutsEnabledForScope(Scope::HelpShortcutScope), runtime.helpShortcutsEnabled());
    QCOMPARE(runtime.shortcutsEnabledForScope(Scope::ReadyViewerShortcutScope),
        runtime.readyViewerShortcutsEnabled());
    QCOMPARE(runtime.shortcutsEnabledForScope(Scope::PannableViewerShortcutScope),
        runtime.pannableViewerShortcutsEnabled());
    QCOMPARE(runtime.shortcutsEnabledForScope(Scope::ContainerShortcutScope),
        runtime.containerShortcutsEnabled());
    QVERIFY(!runtime.shortcutsEnabledForScope(Scope::ImageSelectionShortcutScope));
    QVERIFY(!runtime.shortcutsEnabledForScope(Scope::PageViewerShortcutScope));
}

void TestImageActionAvailabilityRuntime::mediaShortcutScopeLookupCombinesSessionAndVideoInputs()
{
    using Scope = KiriView::ApplicationActions::ImageShortcutScope;

    KiriView::ApplicationActions::ImageActionAvailabilityRuntime runtime;
    runtime.setImageReady(true);

    QVERIFY(runtime.mediaShortcutsEnabledForScope(
        Scope::ImageSelectionShortcutScope, false, true, false));
    QVERIFY(
        runtime.mediaShortcutsEnabledForScope(Scope::PageViewerShortcutScope, true, true, false));
    QVERIFY(
        !runtime.mediaShortcutsEnabledForScope(Scope::PageViewerShortcutScope, true, false, false));
    QVERIFY(
        runtime.mediaShortcutsEnabledForScope(Scope::ReadyViewerShortcutScope, true, true, false));
    QVERIFY(
        !runtime.mediaShortcutsEnabledForScope(Scope::ReadyViewerShortcutScope, true, true, true));
}

QTEST_GUILESS_MAIN(TestImageActionAvailabilityRuntime)

#include "test_imageactionavailabilityruntime.moc"
