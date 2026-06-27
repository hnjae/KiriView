// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/applicationactionhost.h"
#include "application/applicationactionruntime.h"
#include "application/applicationcommandportsource.h"
#include "application/applicationcommandrouter.h"
#include "application/navigationpresentationprojection.h"
#include "session/documentsessiontypes.h"

#include <KirigamiActionCollection>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QObject>
#include <QTest>

namespace {
namespace Actions = kiriview::ApplicationActions;
using ActionId = kiriview::ApplicationActions::ActionId;

class FakeApplicationActionHost final : public Actions::ApplicationActionHost
{
public:
    FakeApplicationActionHost()
        : collection(&object)
    {
    }

    QObject* actionContext() override { return &object; }
    KirigamiActionCollection* mainActionCollection() override { return &collection; }
    QAction* inheritedAction(const QString& actionName) override
    {
        return collection.action(actionName);
    }
    void readActionSettings() override { }

    QObject object;
    KirigamiActionCollection collection;
};

struct CommandLog
{
    int openDialogCount = 0;
    int previousNavigationCount = 0;
};

void compareNavigationSlot(
    const Actions::NavigationPresentationSlot& slot, ActionId actionId, ActionId iconActionId)
{
    QCOMPARE(slot.actionId, actionId);
    QCOMPARE(slot.iconActionId, iconActionId);
}

class FakeCommandPortSource final : public Actions::ApplicationCommandPortSource
{
public:
    Actions::ApplicationCommandRouterShellPorts commandRouterShellPorts() override
    {
        Actions::ApplicationCommandRouterShellPorts ports;
        ports.requestOpenDialog = [this]() { ++log.openDialogCount; };
        return ports;
    }

    Actions::ApplicationCommandRouterSessionPorts commandRouterSessionPorts() override
    {
        Actions::ApplicationCommandRouterSessionPorts ports;
        ports.requestPreviousActiveNavigationWithBoundary
            = [this]() { ++log.previousNavigationCount; };
        return ports;
    }

    CommandLog log;
};
}

class TestApplicationActionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void triggeredActionDispatchesThroughRuntimeOwnedRouter();
    void fixedShortcutDispatchesThroughRuntimeOwnedRouter();
    void actionStateSnapshotBuildsRuntimePolicyInput();
    void actionStateSnapshotBuildsCommandRouterInput();
    void navigationPresentationProjectionFollowsActionStateSnapshot();
};

void TestApplicationActionRuntime::triggeredActionDispatchesThroughRuntimeOwnedRouter()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    FakeCommandPortSource portSource;

    runtime.setCommandPortSource(&portSource);
    runtime.handleActionTriggered(ActionId::FileOpenAction);

    QCOMPARE(portSource.log.openDialogCount, 1);
}

void TestApplicationActionRuntime::fixedShortcutDispatchesThroughRuntimeOwnedRouter()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    FakeCommandPortSource portSource;
    Actions::ApplicationActionStateSnapshot snapshot;
    snapshot.videoMode = true;

    runtime.setCommandPortSource(&portSource);
    runtime.setActionStateSnapshot(snapshot);

    QVERIFY(runtime.executeHorizontalArrowShortcut(true));

    QCOMPARE(portSource.log.previousNavigationCount, 1);
}

void TestApplicationActionRuntime::actionStateSnapshotBuildsRuntimePolicyInput()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    Actions::ApplicationActionStateSnapshot snapshot;
    snapshot.uiGateRevision = 7;
    snapshot.sessionActionAvailability = kiriview::DocumentSessionActionAvailabilityFacts {
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        false,
        false,
    };
    snapshot.activeNavigationDispatchAvailable = true;
    snapshot.activeNavigationAvailable = true;
    snapshot.activeNavigationKnown = true;
    snapshot.activeNavigationHasTargets = true;
    snapshot.canOpenPreviousActiveNavigation = true;
    snapshot.canOpenNextActiveNavigation = true;
    snapshot.displayedMediaOpenWithAvailable = true;
    snapshot.displayedFileDeletionAvailable = true;
    snapshot.imagePannable = true;
    snapshot.applicationMenuShortcutEnabled = true;
    snapshot.showMenubarActionEnabled = false;

    runtime.setActionStateSnapshot(snapshot);

    QCOMPARE(runtime.actionStateRevision(), 1);
    QVERIFY(runtime.actionPlacementEnabled(ActionId::GoPreviousImageAction));
    QVERIFY(runtime.actionPlacementEnabled(ActionId::ViewToggleRightToLeftReadingAction));
    QCOMPARE(runtime.actionMenuText(ActionId::ViewFitAction), QStringLiteral("Fit to &Window"));
    QVERIFY(runtime.rightToLeftReadingActive());
}

void TestApplicationActionRuntime::actionStateSnapshotBuildsCommandRouterInput()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);
    Actions::ApplicationActionStateSnapshot snapshot;
    snapshot.sessionActionAvailability.rightToLeftReadingActive = true;
    snapshot.sessionActionAvailability.rightToLeftReadingAvailable = true;
    snapshot.imagePannable = true;
    snapshot.videoMode = true;
    snapshot.imageDocumentPageNavigationActive = true;
    snapshot.atKnownFirstActiveNavigation = true;
    snapshot.canOpenPreviousActiveNavigation = true;

    runtime.setActionStateSnapshot(snapshot);

    const Actions::ApplicationCommandRouterInput input = runtime.commandRouterInput();
    QVERIFY(input.imagePannable);
    QVERIFY(input.rightToLeftReadingActive);
    QVERIFY(input.videoMode);
    QVERIFY(input.imageDocumentPageNavigationActive);
    QVERIFY(input.atKnownFirstActiveNavigation);
    QVERIFY(input.canOpenPreviousActiveNavigation);
}

void TestApplicationActionRuntime::navigationPresentationProjectionFollowsActionStateSnapshot()
{
    FakeApplicationActionHost host;
    Actions::ApplicationActionRuntime runtime(host);

    Actions::NavigationPresentationProjection projection
        = runtime.navigationPresentationProjection();
    compareNavigationSlot(projection.leadingImageAction, ActionId::GoPreviousImageAction,
        ActionId::GoPreviousImageAction);
    compareNavigationSlot(projection.trailingArchiveMenuAction, ActionId::GoNextArchiveAction,
        ActionId::GoNextArchiveAction);

    Actions::ApplicationActionStateSnapshot snapshot;
    snapshot.sessionActionAvailability.imageReady = true;
    snapshot.sessionActionAvailability.rightToLeftReadingActive = true;
    snapshot.sessionActionAvailability.rightToLeftReadingAvailable = true;

    runtime.setActionStateSnapshot(snapshot);

    projection = runtime.navigationPresentationProjection();
    compareNavigationSlot(projection.leadingImageAction, ActionId::GoNextImageAction,
        ActionId::GoPreviousImageAction);
    compareNavigationSlot(projection.trailingArchiveMenuAction, ActionId::GoPreviousArchiveAction,
        ActionId::GoNextArchiveAction);
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);
    TestApplicationActionRuntime test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_applicationactionruntime.moc"
