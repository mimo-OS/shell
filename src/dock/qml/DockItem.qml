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
import Qt5Compat.GraphicalEffects
import Cutefish.Dock 1.0
import FishUI 1.0 as FishUI

Item {
    id: control

    property bool isLeft: Settings.direction === DockSettings.Left
    property bool isRight: Settings.direction === DockSettings.Right
    property bool isBottom: Settings.direction === DockSettings.Bottom

    property var iconSize: root.isHorizontal ? control.height * iconSizeRatio
                                             : control.width * iconSizeRatio

    property bool draggable: false
    property int dragItemIndex

    property alias icon: icon
    property alias mouseArea: iconArea
    property alias dropArea: iconDropArea

    property bool enableActivateDot: true
    property bool isActive: false

    property var popupText

    property double iconSizeRatio: 0.8
    property var iconName

    property bool dragStarted: false

    signal positionChanged()
    signal released()
    signal pressed(var mouse)
    signal pressAndHold(var mouse)
    signal clicked(var mouse)
    signal rightClicked(var mouse)
    signal doubleClicked(var mouse)
    signal dropped(var drop)

    function updatePopupTipsPosition() {
        if (!iconArea.containsMouse)
            return

        var controlPosition = control.mapToGlobal(0, 0)
        if (Settings.direction === DockSettings.Left)
            popupTips.position = Qt.point(root.mapToGlobal(root.width, 0).x + FishUI.Units.largeSpacing,
                                          controlPosition.y + (control.height / 2 - popupTips.height / 2))
        else if (Settings.direction === DockSettings.Right)
            popupTips.position = Qt.point(root.mapToGlobal(0, 0).x - popupTips.width - FishUI.Units.largeSpacing,
                                          controlPosition.y + (control.height / 2 - popupTips.height / 2))
        else
            popupTips.position = Qt.point(controlPosition.x + (control.width / 2 - popupTips.width / 2),
                                          root.mapToGlobal(0, 0).y - popupTips.height - FishUI.Units.smallSpacing)
    }

    function popupMenu(menu, mouse) {
        var iconPosition = icon.mapToGlobal(0, 0)
        var clickPosition = control.mapToGlobal(mouse.x, mouse.y)
        menu.popupAt(iconPosition.x, clickPosition.y)
    }

    function refreshIcon() {
        if (icon && icon.width > 0 && icon.height > 0)
            icon.updateIcon()
    }

    onIconNameChanged: refreshIcon()
    onIconSizeChanged: refreshIcon()

    Component.onCompleted: refreshIcon()

    Drag.active: mouseArea.drag.active && control.draggable
    Drag.dragType: Drag.Automatic
    Drag.supportedActions: Qt.MoveAction
    Drag.hotSpot.x: icon.width / 2
    Drag.hotSpot.y: icon.height / 2

    Drag.onDragStarted:  {
        dragStarted = true
    }

    Drag.onDragFinished: {
        dragStarted = false
    }

    FishUI.IconImage {
        id: icon
        anchors.centerIn: parent
        width: control.iconSize
        height: control.iconSize
        source: iconName

        visible: !dragStarted

        ColorOverlay {
            id: iconColorize
            anchors.fill: icon
            source: icon
            color: "#000000"
            opacity: iconArea.pressed && !mouseArea.drag.active ? 0.4 : 0
        }
    }

    DropArea {
        id: iconDropArea
        anchors.fill: icon
        enabled: draggable
        onDropped: function(drop) { control.dropped(drop) }
    }

    MouseArea {
        id: iconArea
        anchors.fill: icon
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        drag.axis: Drag.XAndYAxis

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton)
                control.rightClicked(mouse)
            else
                control.clicked(mouse)
        }

        onPressed: function(mouse) {
            control.pressed(mouse)
            popupTips.hide()
        }

        onPositionChanged: function(mouse) {
            if (pressed) {
                if (control.draggable && mouse.source !== Qt.MouseEventSynthesizedByQt) {
                    drag.target = icon
                    icon.grabToImage(function(result) {
                        control.Drag.imageSource = result.url
                    })
                } else {
                    drag.target = null
                }
            }

            control.positionChanged()
        }

        onPressAndHold: function(mouse) { control.pressAndHold(mouse) }
        onReleased: {
            drag.target = null
            control.released()
        }

        onContainsMouseChanged: {
            if (containsMouse && control.popupText !== "") {
                popupTips.popupText = control.popupText
                popupPositionTimer.restart()
            } else {
                popupPositionTimer.stop()
                popupTips.hide()
            }
        }
    }

    Timer {
        id: popupPositionTimer
        interval: 0
        repeat: false
        onTriggered: {
            if (iconArea.containsMouse && popupTips.popupText !== "") {
                if (!popupTips.visible) {
                    popupTips.show()
                    restart()
                } else {
                    control.updatePopupTipsPosition()
                }
            }
        }
    }

    Rectangle {
        id: activeRect

        // The dot sits in the dedicated strip the dock keeps between the icons
        // and the screen edge, outside this delegate.
        property var leftX: -(root.activeDotGap + width)
        property var leftY: (parent.height - height) / 2

        property var bottomX: (parent.width - width) / 2
        property var bottomY: parent.height + root.activeDotGap

        property var rightX: parent.width + root.activeDotGap
        property var rightY: (parent.height - height) / 2

        property var circleSize: root.activeDotSize

        width: circleSize
        height: circleSize
        radius: width / 2
        visible: enableActivateDot && !dragStarted
        color: FishUI.Theme.textColor

        x: isLeft ? leftX : isBottom ? bottomX : rightX
        y: isLeft ? leftY : isBottom ? bottomY : rightY
    }
}
