/*
 * Copyright (C) 2026 CutefishOS Team.
 *
 * WindowPreviewPopup.qml - Live interactive taskbar/dock window preview cards
 */

import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import FishUI 1.0 as FishUI
import Cutefish.Dock 1.0

Window {
    id: previewPopup
    visible: false
    color: "transparent"
    flags: Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.ToolTip

    property string appId: ""
    property string appName: ""
    property var windows: []
    property point popupPosition: Qt.point(0, 0)
    property bool containsCursor: hoverHandler.hovered

    signal windowActivated(string wid)
    signal windowClosed(string wid)

    width: mainCard.implicitWidth
    height: mainCard.implicitHeight

    x: popupPosition.x
    y: popupPosition.y

    FishUI.WindowHelper {
        id: windowHelper
    }

    FishUI.WindowShadow {
        view: previewPopup
        radius: mainCard.radius + 4
        strength: 0.4
    }

    FishUI.WindowBlur {
        view: previewPopup
        enabled: FishUI.Theme.blurEnabled
        windowRadius: mainCard.radius
    }

    HoverHandler {
        id: hoverHandler
    }

    onPopupPositionChanged: adjustCorrectLocation()
    onWidthChanged: adjustCorrectLocation()
    onHeightChanged: adjustCorrectLocation()

    function adjustCorrectLocation() {
        var posX = previewPopup.popupPosition.x
        var posY = previewPopup.popupPosition.y

        if (windowHelper.wayland) {
            previewPopup.x = posX
            previewPopup.y = posY
            return
        }

        if (posX <= Screen.virtualX)
            posX = Screen.virtualX + FishUI.Units.smallSpacing

        if (posY <= Screen.virtualY)
            posY = Screen.virtualY + FishUI.Units.smallSpacing

        if (posX + previewPopup.width > Screen.virtualX + Screen.width)
            posX = Screen.virtualX + Screen.width - previewPopup.width - 1

        if (posY + previewPopup.height > Screen.virtualY + Screen.height)
            posY = Screen.virtualY + Screen.height - previewPopup.height - 1

        previewPopup.x = posX
        previewPopup.y = posY
    }

    function show() {
        if (windowHelper.popupMenuVisible())
            return
        visible = true
    }

    function hide() {
        visible = false
    }

    Rectangle {
        id: mainCard
        anchors.fill: parent
        implicitWidth: windows.length > 0 ? (previewRow.implicitWidth + 20) : (titleLabel.implicitWidth + 24)
        implicitHeight: windows.length > 0 ? (previewRow.implicitHeight + 20) : (titleLabel.implicitHeight + 14)

        radius: 12
        color: FishUI.Theme.darkMode
               ? Qt.rgba(0.12, 0.12, 0.16, FishUI.Theme.blurEnabled ? 0.78 : 0.96)
               : Qt.rgba(0.96, 0.96, 0.98, FishUI.Theme.blurEnabled ? 0.82 : 0.98)
        border.width: 1 / FishUI.Dpi.ratio
        border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.15)
                                            : Qt.rgba(0, 0, 0, 0.12)

        // Simple text label when no running windows exist
        Label {
            id: titleLabel
            anchors.centerIn: parent
            visible: previewPopup.windows.length === 0
            text: previewPopup.appName
            font.bold: true
            font.pointSize: 10
            color: FishUI.Theme.textColor
        }

        // Horizontal row of window preview tiles
        Row {
            id: previewRow
            anchors.centerIn: parent
            spacing: 10
            visible: previewPopup.windows.length > 0

            Repeater {
                model: previewPopup.windows

                Item {
                    id: tileItem
                    width: 176
                    height: 116

                    readonly property var winData: modelData || {}
                    readonly property bool isCurrentActive: winData ? (winData.active === true) : false
                    readonly property bool isMinimized: winData ? (winData.minimized === true) : false
                    property bool hovered: tileMouseArea.containsMouse || closeMouseArea.containsMouse

                    Rectangle {
                        id: tileBg
                        anchors.fill: parent
                        radius: 8
                        color: tileItem.hovered
                               ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.08))
                               : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.04))
                        border.width: tileItem.isCurrentActive ? 2 : 1
                        border.color: tileItem.isCurrentActive
                                      ? FishUI.Theme.highlightColor
                                      : (tileItem.hovered
                                         ? (FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.28) : Qt.rgba(0, 0, 0, 0.20))
                                         : (FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10) : Qt.rgba(0, 0, 0, 0.08)))

                        scale: tileItem.hovered ? 1.02 : 1.0

                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }

                        MouseArea {
                            id: tileMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                previewPopup.windowActivated(winData.wid)
                            }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 7
                            spacing: 6

                            // Top Header: App Icon, Window Title, Close Button
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                FishUI.IconItem {
                                    Layout.preferredWidth: 16
                                    Layout.preferredHeight: 16
                                    source: (winData && winData.iconName) ? winData.iconName : "application-x-desktop"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (winData && winData.title) ? winData.title : ""
                                    font.pixelSize: 11
                                    font.bold: tileItem.isCurrentActive
                                    color: FishUI.Theme.textColor
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }

                                // Close Button (x)
                                Rectangle {
                                    id: closeBtn
                                    z: 10
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 20
                                    radius: 10
                                    color: closeMouseArea.containsMouse
                                           ? "#FF453A"
                                           : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.10))

                                    Behavior on color { ColorAnimation { duration: 100 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "\u00d7"
                                        font.pixelSize: 14
                                        font.bold: true
                                        color: closeMouseArea.containsMouse ? "#FFFFFF" : FishUI.Theme.textColor
                                    }

                                    MouseArea {
                                        id: closeMouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: function(mouse) {
                                            mouse.accepted = true
                                            previewPopup.windowClosed(winData.wid)
                                        }
                                    }
                                }
                            }

                            // Body: Miniature window canvas preview
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 5
                                color: FishUI.Theme.darkMode ? Qt.rgba(0, 0, 0, 0.40) : Qt.rgba(1, 1, 1, 0.65)
                                border.width: 1
                                border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.08) : Qt.rgba(0, 0, 0, 0.06)

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 4

                                    FishUI.IconItem {
                                        Layout.alignment: Qt.AlignHCenter
                                        Layout.preferredWidth: 32
                                        Layout.preferredHeight: 32
                                        source: (winData && winData.iconName) ? winData.iconName : "application-x-desktop"
                                        opacity: tileItem.isMinimized ? 0.45 : 0.85
                                    }

                                    Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        visible: tileItem.isMinimized
                                        text: qsTr("Minimized")
                                        font.pixelSize: 9
                                        color: FishUI.Theme.disabledTextColor
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
