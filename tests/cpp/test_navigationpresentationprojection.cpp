// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "application/navigationpresentationprojection.h"

#include <QObject>
#include <QTest>

namespace {
namespace Actions = kiriview::ApplicationActions;
using ActionId = kiriview::ApplicationActions::ActionId;

void compareSlot(
    const Actions::NavigationPresentationSlot& slot, ActionId actionId, ActionId iconActionId)
{
    QCOMPARE(slot.actionId, actionId);
    QCOMPARE(slot.iconActionId, iconActionId);
}
}

class TestNavigationPresentationProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void leftToRightProjectionKeepsSemanticOrderAndPhysicalIcons();
    void rightToLeftProjectionReordersActionsAndKeepsPhysicalIcons();
};

void TestNavigationPresentationProjection::leftToRightProjectionKeepsSemanticOrderAndPhysicalIcons()
{
    const Actions::NavigationPresentationProjection projection
        = Actions::navigationPresentationProjection(false);

    compareSlot(projection.leadingImageAction, ActionId::GoPreviousImageAction,
        ActionId::GoPreviousImageAction);
    compareSlot(
        projection.trailingImageAction, ActionId::GoNextImageAction, ActionId::GoNextImageAction);
    compareSlot(projection.leadingImageMenuAction, ActionId::GoPreviousImageAction,
        ActionId::GoPreviousImageAction);
    compareSlot(projection.trailingImageMenuAction, ActionId::GoNextImageAction,
        ActionId::GoNextImageAction);
    compareSlot(projection.firstImageMenuAction, ActionId::GoFirstImageAction,
        ActionId::GoFirstImageAction);
    compareSlot(
        projection.lastImageMenuAction, ActionId::GoLastImageAction, ActionId::GoLastImageAction);
    compareSlot(projection.leadingArchiveMenuAction, ActionId::GoPreviousArchiveAction,
        ActionId::GoPreviousArchiveAction);
    compareSlot(projection.trailingArchiveMenuAction, ActionId::GoNextArchiveAction,
        ActionId::GoNextArchiveAction);

    QCOMPARE(projection.applicationMenuArchiveActionIds.at(0), ActionId::GoPreviousArchiveAction);
    QCOMPARE(projection.applicationMenuArchiveActionIds.at(1), ActionId::GoNextArchiveAction);
}

void TestNavigationPresentationProjection::
    rightToLeftProjectionReordersActionsAndKeepsPhysicalIcons()
{
    const Actions::NavigationPresentationProjection projection
        = Actions::navigationPresentationProjection(true);

    compareSlot(projection.leadingImageAction, ActionId::GoNextImageAction,
        ActionId::GoPreviousImageAction);
    compareSlot(projection.trailingImageAction, ActionId::GoPreviousImageAction,
        ActionId::GoNextImageAction);
    compareSlot(projection.leadingImageMenuAction, ActionId::GoNextImageAction,
        ActionId::GoPreviousImageAction);
    compareSlot(projection.trailingImageMenuAction, ActionId::GoPreviousImageAction,
        ActionId::GoNextImageAction);
    compareSlot(
        projection.firstImageMenuAction, ActionId::GoFirstImageAction, ActionId::GoLastImageAction);
    compareSlot(
        projection.lastImageMenuAction, ActionId::GoLastImageAction, ActionId::GoFirstImageAction);
    compareSlot(projection.leadingArchiveMenuAction, ActionId::GoNextArchiveAction,
        ActionId::GoPreviousArchiveAction);
    compareSlot(projection.trailingArchiveMenuAction, ActionId::GoPreviousArchiveAction,
        ActionId::GoNextArchiveAction);

    QCOMPARE(projection.applicationMenuArchiveActionIds.at(0), ActionId::GoNextArchiveAction);
    QCOMPARE(projection.applicationMenuArchiveActionIds.at(1), ActionId::GoPreviousArchiveAction);
}

QTEST_GUILESS_MAIN(TestNavigationPresentationProjection)

#include "test_navigationpresentationprojection.moc"
