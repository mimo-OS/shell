/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * macOS Notification Banner Popup
 */

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects
import FishUI 1.0 as FishUI
import Cutefish.Notification 1.0

NotificationPopup {
    id: control
    flags: Qt.WindowDoesNotAcceptFocus | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    width: 340
    height: Math.max(68, _contentLayout.implicitHeight + 20)
    color: "transparent"
    visible: false
    onVisibleChanged: if (visible) timer.restart()

    property int defaultTimeout: 6000

    FishUI.WindowShadow {
        view: control
        radius: _background.radius
    }

    FishUI.WindowBlur {
        view: control
        windowRadius: _background.radius
        enabled: FishUI.Theme.blurEnabled
    }

    Rectangle {
        id: _background
        anchors.fill: parent
        radius: 16
        color: FishUI.Theme.darkMode ? Qt.rgba(0.14, 0.14, 0.16, FishUI.Theme.blurEnabled ? 0.76 : 1.0)
                                     : Qt.rgba(0.96, 0.96, 0.98, FishUI.Theme.blurEnabled ? 0.78 : 1.0)
        border.width: 1 / FishUI.Dpi.ratio
        border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.15) : Qt.rgba(0, 0, 0, 0.10)
    }

    MouseArea {
        id: _mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onEntered: timer.stop()
        onExited: timer.restart()

        onClicked: {
            if (model.hasDefaultAction) {
                notificationsModel.invokeDefaultAction(model.notificationId)
            }
            notificationsModel.close(model.notificationId)
        }
    }

    ColumnLayout {
        id: _contentLayout
        anchors.fill: parent
        anchors.margins: 10
        spacing: 4

        // Top Row: App Icon, App Name, Timestamp, Close Button
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Image {
                id: _icon
                width: 18
                height: 18
                source: model.iconName ? "image://icontheme/%1".arg(model.iconName) : ""
                sourceSize: Qt.size(18, 18)
                Layout.alignment: Qt.AlignVCenter
                antialiasing: true
                smooth: true
                visible: status === Image.Ready
            }

            Image {
                id: _defaultIcon
                width: 18
                height: 18
                source: "image://icontheme/preferences-desktop-notification"
                sourceSize: Qt.size(18, 18)
                Layout.alignment: Qt.AlignVCenter
                antialiasing: true
                smooth: true
                visible: !_icon.visible
            }

            Label {
                text: (model.appName && model.appName.length > 0) ? model.appName : qsTr("Notification")
                font.bold: true
                font.pointSize: 8.5
                color: FishUI.Theme.textColor
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Label {
                text: qsTr("now")
                font.pointSize: 7.5
                color: FishUI.Theme.disabledTextColor
                Layout.alignment: Qt.AlignVCenter
            }

            // Close button
            Rectangle {
                width: 16
                height: 16
                radius: 8
                color: _closeBtnArea.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.15))
                                                   : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.06))
                visible: _mouseArea.containsMouse || _closeBtnArea.containsMouse
                antialiasing: true

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    font.pixelSize: 12
                    font.bold: true
                    color: FishUI.Theme.textColor
                }

                MouseArea {
                    id: _closeBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: timer.stop()
                    onClicked: notificationsModel.close(model.notificationId)
                }
            }
        }

        // Summary Title
        Label {
            text: model.summary
            visible: text.length > 0
            font.bold: true
            font.pointSize: 9
            color: FishUI.Theme.textColor
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        // Body Text
        Label {
            text: model.body
            visible: text.length > 0
            font.pointSize: 8
            color: FishUI.Theme.textColor
            opacity: 0.8
            maximumLineCount: 2
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }

    Timer {
        id: timer
        interval: control.defaultTimeout
        onTriggered: notificationsModel.expired(model.notificationId)
    }
}
