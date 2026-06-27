// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml
import org.hnjae.kiriview

QtObject {
    id: root

    required property var actions
    required property var projectionProvider
    readonly property int projectionRevision: root.projectionProvider?.actionStateRevision ?? 0

    readonly property var actionLookup: {
        const lookup = {};
        lookup[KiriViewApplication.GoPreviousArchiveAction] = root.actions.previousContainerAction;
        lookup[KiriViewApplication.GoNextArchiveAction] = root.actions.nextContainerAction;
        lookup[KiriViewApplication.GoPreviousImageAction] = root.actions.previousImageAction;
        lookup[KiriViewApplication.GoNextImageAction] = root.actions.nextImageAction;
        lookup[KiriViewApplication.GoFirstImageAction] = root.actions.firstImageAction;
        lookup[KiriViewApplication.GoLastImageAction] = root.actions.lastImageAction;
        return lookup;
    }
    readonly property var menuActionLookup: {
        const lookup = {};
        lookup[KiriViewApplication.GoPreviousArchiveAction] = root.actions.previousContainerMenuAction;
        lookup[KiriViewApplication.GoNextArchiveAction] = root.actions.nextContainerMenuAction;
        lookup[KiriViewApplication.GoPreviousImageAction] = root.actions.previousImageMenuAction;
        lookup[KiriViewApplication.GoNextImageAction] = root.actions.nextImageMenuAction;
        lookup[KiriViewApplication.GoFirstImageAction] = root.actions.firstImageMenuAction;
        lookup[KiriViewApplication.GoLastImageAction] = root.actions.lastImageMenuAction;
        return lookup;
    }

    readonly property var leadingImageAction: root.actionForSlot(KiriViewApplication.LeadingImageActionSlot)
    readonly property var trailingImageAction: root.actionForSlot(KiriViewApplication.TrailingImageActionSlot)
    readonly property string leadingImageIconName: root.iconNameForSlot(KiriViewApplication.LeadingImageActionSlot, false)
    readonly property string trailingImageIconName: root.iconNameForSlot(KiriViewApplication.TrailingImageActionSlot, false)

    readonly property var leadingImageMenuAction: root.menuActionForSlot(KiriViewApplication.LeadingImageMenuActionSlot)
    readonly property var trailingImageMenuAction: root.menuActionForSlot(KiriViewApplication.TrailingImageMenuActionSlot)
    readonly property string leadingImageMenuIconName: root.iconNameForSlot(KiriViewApplication.LeadingImageMenuActionSlot, true)
    readonly property string trailingImageMenuIconName: root.iconNameForSlot(KiriViewApplication.TrailingImageMenuActionSlot, true)

    readonly property var firstImageMenuAction: root.menuActionForSlot(KiriViewApplication.FirstImageMenuActionSlot)
    readonly property var lastImageMenuAction: root.menuActionForSlot(KiriViewApplication.LastImageMenuActionSlot)
    readonly property string firstImageMenuIconName: root.iconNameForSlot(KiriViewApplication.FirstImageMenuActionSlot, true)
    readonly property string lastImageMenuIconName: root.iconNameForSlot(KiriViewApplication.LastImageMenuActionSlot, true)

    readonly property var leadingArchiveMenuAction: root.menuActionForSlot(KiriViewApplication.LeadingArchiveMenuActionSlot)
    readonly property var trailingArchiveMenuAction: root.menuActionForSlot(KiriViewApplication.TrailingArchiveMenuActionSlot)
    readonly property string leadingArchiveMenuIconName: root.iconNameForSlot(KiriViewApplication.LeadingArchiveMenuActionSlot, true)
    readonly property string trailingArchiveMenuIconName: root.iconNameForSlot(KiriViewApplication.TrailingArchiveMenuActionSlot, true)
    readonly property var applicationMenuNavigationActions: root.navigationApplicationMenuActionIds().map(actionId => root.menuActionForId(actionId)).filter(action => action !== null)

    function actionForId(actionId) {
        return root.actionLookup[actionId] ?? null;
    }

    function menuActionForId(actionId) {
        return root.menuActionLookup[actionId] ?? null;
    }

    function projectedActionId(slot) {
        root.projectionRevision;
        return root.projectionProvider.navigationPresentationActionId(slot);
    }

    function projectedIconActionId(slot) {
        root.projectionRevision;
        return root.projectionProvider.navigationPresentationIconActionId(slot);
    }

    function navigationApplicationMenuActionIds() {
        root.projectionRevision;
        return root.projectionProvider.navigationApplicationMenuActionIds();
    }

    function actionForSlot(slot) {
        return root.actionForId(projectedActionId(slot));
    }

    function menuActionForSlot(slot) {
        return root.menuActionForId(projectedActionId(slot));
    }

    function iconNameForSlot(slot, menuAction) {
        const action = menuAction ? root.menuActionForId(projectedIconActionId(slot)) : root.actionForId(projectedIconActionId(slot));
        return action?.icon.name ?? "";
    }
}
