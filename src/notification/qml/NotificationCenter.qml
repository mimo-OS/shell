/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * Rebuilt macOS Notification Center Clone (Pre-Liquid Glass / Big Sur - Sonoma)
 */

import QtQuick 2.12
import QtQml 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects
import FishUI 1.0 as FishUI
import Cutefish.Notification 1.0
import Cutefish.Shell 1.0

PopupWindow {
    id: control

    property var margin: 6
    property point position: Qt.point(StatusBar.screenRect.x + StatusBar.screenRect.width, 0)

    readonly property rect screenRect: {
        return screen.availableScreenRect
    }

    width: 350
    height: screenRect.height - control.margin * 2

    color: "transparent"

    onOpened: historyModel.updateTime()

    onWidthChanged: adjustCorrectLocation()
    onHeightChanged: adjustCorrectLocation()
    onPositionChanged: adjustCorrectLocation()
    onScreenRectChanged: adjustCorrectLocation()

    function adjustCorrectLocation() {
        var posX = control.position.x

        if (posX + control.width >= StatusBar.screenRect.x + StatusBar.screenRect.width)
            posX = StatusBar.screenRect.x + StatusBar.screenRect.width - control.width - control.margin

        control.x = posX
        control.y = control.screenRect.y + control.margin
    }

    Component.onCompleted: adjustCorrectLocation()

    ScreenHelper {
        id: screen
    }

    Rectangle {
        id: _background
        anchors.fill: parent
        radius: 26
        color: FishUI.Theme.darkMode ? Qt.rgba(0.12, 0.14, 0.20, 0.68)
                                     : Qt.rgba(0.92, 0.94, 0.98, 0.72)
        antialiasing: true
        border.width: 1 / FishUI.Dpi.ratio
        border.color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.28)
                                            : Qt.rgba(1, 1, 1, 0.95)

        // Inner Specular Glass Rim
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 25
            color: "transparent"
            border.width: 1 / FishUI.Dpi.ratio
            border.color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.10)
                                                : Qt.rgba(1, 1, 1, 0.50)
        }
    }

    FishUI.WindowShadow {
        view: control
        radius: _background.radius
    }

    FishUI.WindowBlur {
        view: control
        windowRadius: _background.radius
        enabled: FishUI.Theme.blurEnabled
    }

    NumberAnimation {
        id: scrollToTopAni
        target: _view
        from: 0
        to: 0
        property: "contentY"
        duration: 200
        easing.type: Easing.OutSine
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // ==========================================
        // 1. macOS Date Header & Calendar Widget Card
        // ==========================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                id: dateHeaderLabel
                text: new Date().toLocaleDateString(Qt.locale(), "dddd, MMMM d").toUpperCase()
                font.weight: Font.DemiBold
                font.pointSize: 8
                font.letterSpacing: 0.3
                renderType: Text.NativeRendering
                color: FishUI.Theme.disabledTextColor
                Layout.leftMargin: 4
            }

            // Calendar Widget Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 68
                radius: 14
                color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.75)
                border.width: 1 / FishUI.Dpi.ratio
                border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10) : Qt.rgba(0, 0, 0, 0.05)
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    // Mini Calendar Icon
                    Rectangle {
                        width: 48
                        height: 48
                        radius: 10
                        color: "#FFFFFF"
                        clip: true
                        antialiasing: true

                        // Red top ribbon
                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 14
                            color: "#FF3B30"

                            Label {
                                anchors.centerIn: parent
                                text: new Date().toLocaleDateString(Qt.locale(), "MMM").toUpperCase()
                                font.bold: true
                                font.pointSize: 6.5
                                color: "#FFFFFF"
                            }
                        }

                        // Day number
                        Label {
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: 6
                            text: new Date().getDate()
                            font.bold: true
                            font.family: "SF Pro Rounded"
                            font.pointSize: 16
                            color: "#1D1D1F"
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Layout.alignment: Qt.AlignVCenter

                        Label {
                            text: new Date().toLocaleDateString(Qt.locale(), "dddd")
                            font.bold: true
                            font.pointSize: 10
                            color: FishUI.Theme.textColor
                        }

                        Label {
                            text: qsTr("No Upcoming Events")
                            font.pointSize: 8
                            color: FishUI.Theme.disabledTextColor
                        }
                    }
                }
            }
        }

        // ==========================================
        // 2. Notifications Section Header & Clear All Button
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.rightMargin: 4

            Label {
                text: qsTr("Notifications")
                font.weight: Font.DemiBold
                font.pointSize: 13
                font.letterSpacing: 0.2
                renderType: Text.NativeRendering
                color: FishUI.Theme.textColor
                Layout.fillWidth: true

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (_view.contentY === 0)
                            return
                        scrollToTopAni.from = _view.contentY
                        scrollToTopAni.to = 0
                        scrollToTopAni.restart()
                    }
                }
            }

            // macOS Style Clear All Pill Button
            Rectangle {
                visible: _view.count > 0
                Layout.preferredHeight: 22
                Layout.preferredWidth: clearLabel.implicitWidth + 16
                radius: 11
                color: clearMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.15))
                                          : (clearMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0, 0, 0, 0.08))
                                                                      : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.05)))
                border.width: 1 / FishUI.Dpi.ratio
                border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10) : Qt.rgba(0, 0, 0, 0.05)
                antialiasing: true

                Label {
                    id: clearLabel
                    anchors.centerIn: parent
                    text: qsTr("Clear All")
                    font.bold: true
                    font.pointSize: 7.5
                    color: FishUI.Theme.textColor
                }

                MouseArea {
                    id: clearMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: historyModel.clearAll()
                }
            }
        }

        // ==========================================
        // 3. Notification Cards List
        // ==========================================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: _view
                anchors.fill: parent
                model: historyModel
                spacing: 10
                clip: true

                ScrollBar.vertical: ScrollBar {}

                // Empty State
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: _view.count === 0
                    spacing: 8

                    Image {
                        Layout.alignment: Qt.AlignHCenter
                        width: 36
                        height: 36
                        sourceSize: Qt.size(36, 36)
                        source: "image://icontheme/preferences-desktop-notification"
                        opacity: 0.35
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("No Notifications")
                        color: FishUI.Theme.disabledTextColor
                        font.pointSize: 10.5
                    }
                }

                removeDisplaced: Transition {
                    NumberAnimation { properties: "x, y"; duration: 250 }
                }

                delegate: Rectangle {
                    id: notifCard
                    width: ListView.view.width
                    implicitHeight: cardContent.implicitHeight + 20
                    radius: 14
                    color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.80)
                    border.width: 1 / FishUI.Dpi.ratio
                    border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10) : Qt.rgba(0, 0, 0, 0.05)
                    antialiasing: true

                    MouseArea {
                        id: cardHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }

                    ColumnLayout {
                        id: cardContent
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        // Header Row: App icon, App name, timestamp, close button
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
                                text: model.created ? model.created : ""
                                font.pointSize: 7.5
                                color: FishUI.Theme.disabledTextColor
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // Dismiss button
                            Rectangle {
                                width: 16
                                height: 16
                                radius: 8
                                color: closeMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.15))
                                                                : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.06))
                                visible: cardHoverArea.containsMouse
                                antialiasing: true

                                Text {
                                    anchors.centerIn: parent
                                    text: "×"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: FishUI.Theme.textColor
                                }

                                MouseArea {
                                    id: closeMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: historyModel.remove(index)
                                }
                            }
                        }

                        // Summary & Body
                        Label {
                            text: model.summary
                            visible: text.length > 0
                            font.bold: true
                            font.pointSize: 9
                            color: FishUI.Theme.textColor
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: model.body
                            visible: text.length > 0
                            font.pointSize: 8
                            color: FishUI.Theme.textColor
                            opacity: 0.8
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
