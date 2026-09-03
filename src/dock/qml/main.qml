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

    DropArea {
        anchors.fill: parent
        enabled: true
    }

    // Background
    Rectangle {
        id: _background

        property var borderColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.3)
                                                        : Qt.rgba(0, 0, 0, 0.2)

        anchors.fill: parent
        radius: windowRadius
        color: FishUI.Theme.darkMode ? "#666666" : "#E6E6E6"
        opacity: FishUI.Theme.blurEnabled ? 0.5 : 1
        border.width: 1 / FishUI.Dpi.ratio
        border.pixelAligned: FishUI.Dpi.ratio > 1 ? false : true
        border.color: borderColor

        Behavior on color {
            ColorAnimation {
                duration: 200
                easing.type: Easing.Linear
            }
        }
    }

    FishUI.WindowShadow {
        view: dock
        strength: 1
        radius: _background.radius
    }

    FishUI.WindowBlur {
        view: dock
        windowRadius: _background.radius
        enabled: FishUI.Theme.blurEnabled
    }

    FishUI.PopupTips {
        id: popupTips
        backgroundColor: _background.color
        blurEnabled: FishUI.Theme.blurEnabled
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
            // The running dot is drawn outside the delegate, in the dock's
            // own padding area next to the screen edge.
            clip: false

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: root.isHorizontal
                                  ? Math.max(0, root.width - 2 * root.lengthPadding
                                             - root.iconGap - root.itemSize)
                                  : -1
            Layout.preferredHeight: root.isHorizontal
                                   ? -1
                                   : Math.max(0, root.height - 2 * root.lengthPadding
                                              - root.iconGap - root.itemSize)

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

                // The dock exposes one eased progress value for the window,
                // the new slot, the icon scale, and its opacity. Keeping all
                // four bound to that value makes the icon grow from its own
                // center while the panel makes exactly enough room for it.
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

        DockItem {
            id: trashItem
            implicitWidth: root.itemSize
            implicitHeight: root.itemSize
            iconSizeRatio: root.iconSizeRatio
            popupText: qsTr("Trash")
            enableActivateDot: false
            iconName: trash.count === 0 ? "user-trash-empty" : "user-trash-full"
            onClicked: trash.openTrash()
            onRightClicked: function(mouse) { popupMenu(trashMenu, mouse) }

            dropArea.enabled: true

            onDropped: function(drop) {
                if (drop.hasUrls) {
                    trash.moveToTrash(drop.urls)
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: FishUI.Units.smallSpacing / 2
                color: "transparent"
                border.color: FishUI.Theme.textColor
                radius: height * 0.3
                border.width: 1 / FishUI.Dpi.ratio
                border.pixelAligned: FishUI.Dpi.ratio > 1 ? false : true
                opacity: trashItem.dropArea.containsDrag ? 0.5 : 0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                    }
                }
            }

            ScreenHelper {
                id: _screenHelper
            }

            FishUI.DesktopMenu {
                id: trashMenu
                availableGeometry: _screenHelper.availableScreenRect
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
