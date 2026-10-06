/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     rekols <revenmartin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt5Compat.GraphicalEffects

import Cutefish.Dock 1.0
import Cutefish.Shell 1.0
import FishUI 1.0 as FishUI

Item {
    id: root
    visible: true

    property bool isLeft: Settings.direction === DockSettings.Left
    property bool isRight: Settings.direction === DockSettings.Right
    property bool isHorizontal: Settings.direction === DockSettings.Bottom

    // Geometry shared with Dock, see src/dockmetrics.h. The dock is a
    // row (or column) of icons with padding at both ends, and the running dot
    // lives between the icons and the screen edge.
    property var metrics: dock.metrics
    property int iconGap: metrics.iconGap
    property int lengthPadding: metrics.lengthPadding
    property int outerPadding: metrics.outerPadding
    property int activeDotGap: metrics.dotGap
    property int activeDotSize: metrics.dotSize
    property int edgePadding: metrics.edgePadding
    property int dotAreaSize: edgePadding + activeDotSize + activeDotGap
    property int thickness: isHorizontal ? root.height : root.width
    property real windowRadius: thickness * 28 / 86
    property real iconSizeRatio: 1.0
    property int itemSize: Math.max(1, thickness - root.dotAreaSize - root.outerPadding)

    property real hoverPos: -1
    property bool isHovered: false
    property real maxMagnification: 1.40
    property real magnificationSpread: root.itemSize * 2.3
    property int dividerSlotWidth: 14

    ScreenHelper {
        id: _screenHelper
    }

    // Item positions come out in surface coordinates: a plasma-shell panel is
    // reported at (0, 0) on Wayland. A menu clamps its position against this
    // rectangle, so hand it the work area in that same space.
    function menuAvailableGeometry() {
        var origin = root.mapToGlobal(0, 0)
        var rect = _screenHelper.availableScreenRect
        return Qt.rect(rect.x + origin.x - dock.surfacePosition.x,
                       rect.y + origin.y - dock.surfacePosition.y,
                       rect.width, rect.height)
    }

    DropArea {
        anchors.fill: parent
        enabled: true
    }

    // Theme-tinted translucent dock surface.
    Rectangle {
        id: _background

        anchors.fill: parent
        radius: root.windowRadius
        color: FishUI.Theme.darkMode
               ? Qt.rgba(0.08, 0.08, 0.12, FishUI.Theme.blurEnabled ? 0.68 : 0.98)
               : Qt.rgba(0.96, 0.96, 0.98, FishUI.Theme.blurEnabled ? 0.72 : 0.98)
        border.width: 1 / FishUI.Dpi.ratio
        border.pixelAligned: FishUI.Dpi.ratio > 1 ? false : true
        border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.20)
                                            : Qt.rgba(255, 255, 255, 0.65)

        // macOS Sequoia Specular Inner Glass Highlight Rim
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Math.max(0, root.windowRadius - 1)
            color: "transparent"
            border.width: 1 / FishUI.Dpi.ratio
            border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.08)
                                                : Qt.rgba(255, 255, 255, 0.40)
        }
    }

    FishUI.WindowShadow {
        view: dock
        strength: 0.55
        radius: _background.radius + 4
    }

    FishUI.WindowBlur {
        view: dock
        windowRadius: _background.radius
        enabled: FishUI.Theme.blurEnabled
    }

    // Theme-aware dock tooltip.
    FishUI.PopupTips {
        id: popupTips
        backgroundColor: FishUI.Theme.darkMode ? Qt.rgba(0.12, 0.12, 0.14, 0.90)
                                               : Qt.rgba(0.18, 0.18, 0.22, 0.90)
        borderColor: Qt.rgba(255, 255, 255, 0.20)
        blurEnabled: FishUI.Theme.blurEnabled
    }

    // Non-blocking hover tracker for continuous magnification wave across dock
    MouseArea {
        id: backgroundHoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onPositionChanged: function(mouse) {
            var p = mapToItem(mainLayout, mouse.x, mouse.y)
            root.hoverPos = root.isHorizontal ? p.x : p.y
            root.isHovered = true
            hoverResetTimer.restart()
        }
        onExited: {
            hoverResetTimer.restart()
        }
    }

    Timer {
        id: hoverResetTimer
        interval: 90
        repeat: false
        onTriggered: {
            root.isHovered = false
            root.hoverPos = -1
        }
    }

    GridLayout {
        id: mainLayout
        anchors.fill: parent
        // The dot area is always on the side that faces the screen edge.
        anchors.leftMargin: isHorizontal ? root.lengthPadding
                                         : isLeft ? root.dotAreaSize : root.outerPadding
        anchors.rightMargin: isHorizontal ? root.lengthPadding
                                          : isRight ? root.dotAreaSize : root.outerPadding
        anchors.topMargin: isHorizontal ? root.outerPadding : root.lengthPadding
        anchors.bottomMargin: isHorizontal ? root.dotAreaSize : root.lengthPadding
        flow: isHorizontal ? Grid.LeftToRight : Grid.TopToBottom
        columnSpacing: isHorizontal ? root.iconGap : 0
        rowSpacing: isHorizontal ? 0 : root.iconGap

        ListView {
            id: appItemView
            orientation: isHorizontal ? Qt.Horizontal : Qt.Vertical
            spacing: root.iconGap
            snapMode: ListView.SnapToItem
            interactive: false
            model: appModel
            clip: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: root.isHorizontal
                                  ? Math.max(0, root.width - 2 * root.lengthPadding
                                             - root.iconGap - root.itemSize - root.dividerSlotWidth)
                                  : -1
            Layout.preferredHeight: root.isHorizontal
                                   ? -1
                                   : Math.max(0, root.height - 2 * root.lengthPadding
                                              - root.iconGap - root.itemSize - root.dividerSlotWidth)

            delegate: AppItem {
                id: appItemDelegate
                property bool shouldAnimateOnAdd: dock.geometryAnimationAdding
                                                 && model.animationToken === dock.geometryAnimationToken
                property real addProgress: shouldAnimateOnAdd
                                          ? dock.geometryAnimationProgress : 1.0
                property real layoutProgress: addProgress
                property real layoutWidth: root.itemSize * layoutProgress
                property real layoutHeight: root.itemSize * layoutProgress

                width: root.isHorizontal ? layoutWidth : root.itemSize
                height: root.isHorizontal ? root.itemSize : layoutHeight
                scale: addProgress
                transformOrigin: Item.Center
                opacity: addProgress
                iconSizeRatio: root.iconSizeRatio
            }

            // Let the icon disappear into itself instead of vanishing when a
            // window closes or an application is unpinned.
            remove: Transition {
                ParallelAnimation {
                    NumberAnimation {
                        property: "layoutProgress"
                        to: 0.0
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                    NumberAnimation {
                        property: "scale"
                        to: 0.0
                        duration: 180
                        easing.type: Easing.InCubic
                    }
                    NumberAnimation {
                        property: "opacity"
                        to: 0.0
                        duration: 140
                        easing.type: Easing.InCubic
                    }
                }
            }

            addDisplaced: Transition {
                NumberAnimation {
                    properties: "x, y"
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }

            removeDisplaced: Transition {
                NumberAnimation {
                    properties: "x, y"
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }

            moveDisplaced: Transition {
                NumberAnimation {
                    properties: "x, y"
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }

        // macOS Glass Divider
        Item {
            id: dockDivider
            implicitWidth: root.isHorizontal ? root.dividerSlotWidth : root.itemSize
            implicitHeight: root.isHorizontal ? root.itemSize : root.dividerSlotWidth
            Layout.alignment: Qt.AlignCenter

            Rectangle {
                anchors.centerIn: parent
                width: root.isHorizontal ? 1 : parent.width * 0.58
                height: root.isHorizontal ? parent.height * 0.58 : 1
                radius: 0.5
                color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.18)
                                             : Qt.rgba(0, 0, 0, 0.18)

                Rectangle {
                    width: root.isHorizontal ? 1 : parent.width
                    height: root.isHorizontal ? parent.height : 1
                    x: root.isHorizontal ? 1 : 0
                    y: root.isHorizontal ? 0 : 1
                    color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.06)
                                                 : Qt.rgba(255, 255, 255, 0.5)
                }
            }
        }

        DockItem {
            id: trashItem
            implicitWidth: root.itemSize
            implicitHeight: root.itemSize
            iconSizeRatio: root.iconSizeRatio
            popupText: qsTr("Trash")
            enableActivateDot: false
            iconName: trash.count === 0 ? "user-trash-empty" : "user-trash-full"
            onClicked: trash.openTrash()
            onRightClicked: popupMenu(trashMenu)

            dropArea.enabled: true

            onDropped: function(drop) {
                if (drop.hasUrls) {
                    trash.moveToTrash(drop.urls)
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: FishUI.Units.smallSpacing / 2
                color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.12)
                                             : Qt.rgba(0, 0, 0, 0.08)
                border.color: FishUI.Theme.highlightColor
                radius: height * 0.28
                border.width: 1.5
                opacity: trashItem.dropArea.containsDrag ? 0.9 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 180
                    }
                }
            }

            FishUI.DesktopMenu {
                id: trashMenu
                blurEnabled: FishUI.Theme.blurEnabled

                MenuItem {
                    text: qsTr("Open")
                    onTriggered: trash.openTrash()
                }

                MenuItem {
                    text: qsTr("Empty Trash")
                    onTriggered: trash.emptyTrash()
                    visible: trash.count !== 0
                }
            }
        }
    }

    Connections {
        target: Settings

        function onDirectionChanged() {
            popupTips.hide()
        }
    }

    Connections {
        target: dock

        function onVisibleChanged() {
            popupTips.hide()
        }
    }
}
