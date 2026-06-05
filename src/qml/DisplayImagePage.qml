// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    property var displaySource: null

    signal loadOutcomeAcknowledged(url providerUrl, string revisionToken, string sourceIdentity, int outcome)

    readonly property int loadedOutcome: 0
    readonly property int errorOutcome: 1
    readonly property int missingOutcome: 2
    readonly property bool hasDisplaySource: displaySource !== null
    readonly property url providerUrl: hasDisplaySource ? displaySource.providerUrl : ""
    readonly property string revisionToken: hasDisplaySource ? displaySource.revisionToken : ""
    readonly property string sourceIdentity: hasDisplaySource ? displaySource.sourceIdentity : ""
    readonly property size sourceSizeHint: hasDisplaySource ? displaySource.sourceSizeHint : Qt.size(0, 0)
    readonly property bool loadAcknowledgmentRequired: hasDisplaySource && displaySource.loadAcknowledgmentRequired
    readonly property string acknowledgementKey: providerUrl.toString() + "\n" + revisionToken + "\n" + sourceIdentity
    property string acknowledgedKey: ""
    property bool acknowledgementQueued: false

    visible: hasDisplaySource && displaySource.visible

    function queueLoadAcknowledgment() {
        if (acknowledgementQueued) {
            return;
        }

        acknowledgementQueued = true;
        Qt.callLater(root.acknowledgeCurrentLoadOutcome);
    }

    function acknowledgeCurrentLoadOutcome() {
        acknowledgementQueued = false;

        if (!loadAcknowledgmentRequired || acknowledgedKey === acknowledgementKey) {
            return;
        }

        if (providerUrl.toString() === "") {
            acknowledgedKey = acknowledgementKey;
            loadOutcomeAcknowledged(providerUrl, revisionToken, sourceIdentity, missingOutcome);
            return;
        }

        switch (providerImage.status) {
        case Image.Ready:
            acknowledgedKey = acknowledgementKey;
            loadOutcomeAcknowledged(providerUrl, revisionToken, sourceIdentity, loadedOutcome);
            return;
        case Image.Error:
            acknowledgedKey = acknowledgementKey;
            loadOutcomeAcknowledged(providerUrl, revisionToken, sourceIdentity, errorOutcome);
            return;
        default:
            return;
        }
    }

    onAcknowledgementKeyChanged: {
        acknowledgedKey = "";
        queueLoadAcknowledgment();
    }
    onLoadAcknowledgmentRequiredChanged: queueLoadAcknowledgment()

    Image {
        id: providerImage

        objectName: "providerImage"
        anchors.fill: parent
        asynchronous: false
        cache: root.hasDisplaySource && root.displaySource.cacheEnabled
        fillMode: Image.PreserveAspectFit
        mipmap: true
        retainWhileLoading: root.hasDisplaySource && root.displaySource.retainWhileLoadingEligible
        rotation: root.hasDisplaySource ? root.displaySource.rotationDegrees : 0
        smooth: true
        source: root.providerUrl
        sourceSize: root.sourceSizeHint
        visible: root.visible && root.providerUrl.toString() !== ""

        onStatusChanged: root.queueLoadAcknowledgment()
    }

    Component.onCompleted: queueLoadAcknowledgment()
}
