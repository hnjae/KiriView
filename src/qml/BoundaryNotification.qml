// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    id: root

    property bool active: false
    property string message: ""

    function show(message) {
        if (!message) {
            return;
        }

        root.message = message;
        root.active = true;
        dismissTimer.restart();
    }

    function dismiss() {
        dismissTimer.stop();
        root.active = false;
        root.message = "";
    }

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    Kirigami.Theme.inherit: false

    implicitWidth: notification.implicitWidth
    implicitHeight: notification.implicitHeight + Kirigami.Units.largeSpacing
    width: implicitWidth
    height: implicitHeight
    visible: root.active

    Timer {
        id: dismissTimer

        interval: 7000
        repeat: false

        onTriggered: root.dismiss()
    }

    Controls.Control {
        id: notification

        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        leftPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.largeSpacing
        topPadding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing
        implicitWidth: Math.min(implicitContentWidth + leftPadding + rightPadding, Math.min(Kirigami.Units.gridUnit * 25, root.parent ? root.parent.width / 1.5 : Kirigami.Units.gridUnit * 25))

        contentItem: Controls.Label {
            color: Kirigami.Theme.textColor
            elide: Text.ElideRight
            text: root.message
            wrapMode: Text.Wrap
        }

        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
            opacity: 0.9
            radius: Kirigami.Units.cornerRadius
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton

            onTapped: root.dismiss()
        }
    }
}
