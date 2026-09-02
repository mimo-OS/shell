/*
 * Copyright (C) 2021 CutefishOS.
 *
 * Author:     Reoin Wong <reion@cutefishos.com>
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
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import Cutefish.Shell 1.0
import FishUI 1.0 as FishUI
import Cutefish.Launcher 1.0

Item {
    id: control

    property bool searchMode: false

    property int pageIndex: 0
    property int pageCount: 0

    property int iconSize: 96
    property int labelPixelSize: 13
    property int labelHeight: 32
    property int spacing: 12

    property var dragLayer: null

    readonly property int itemIndex: index
    readonly property bool dragging: iconMouseArea.drag.active

    z: dragging ? 2 : 0

    IconItem {
        id: icon

        width: control.iconSize
        height: control.iconSize

        x: Math.round((control.width - width) / 2)
        y: Math.round((control.height - height - control.labelHeight - control.spacing) / 2)

        source: model.iconName
        visible: !control.dragging

        ColorOverlay {
            anchors.fill: icon
            source: icon
            color: "#000000"
            opacity: 0.5
            visible: iconMouseArea.pressed && !control.dragging
        }
    }

    DropArea {
        id: dropArea

        anchors.fill: parent
        keys: ["cutefish-launcher"]
        enabled: !control.searchMode

        onEntered: reorderTimer.restart()
        onExited: reorderTimer.stop()

        // The dwell below already reordered; the drop just ends the drag.
        onDropped: function (drop) {
            reorderTimer.stop()
            drop.accept()
        }
    }

    // Hovering a neighbour long enough swaps it, rather than waiting for the
    // button to come up.
    Timer {
        id: reorderTimer

        interval: 150

        onTriggered: {
            if (!dropArea.containsDrag || !control.dragLayer)
                return

            // Wait out the previous swap's animation.
            if (control.dragLayer.dragProxy.settling) {
                reorderTimer.restart()
                return
            }

            var source = dropArea.drag.source
            if (!source || source === control)
                return

            launcherModel.move(source.itemIndex, control.itemIndex,
                               control.pageIndex, control.pageCount)
            control.dragLayer.dragProxy.startSettle()
        }
    }

    ScreenHelper {
        id: _screenHelper
    }

    FishUI.DesktopMenu {
        id: _itemMenu
        availableGeometry: _screenHelper.availableScreenRect
        blurEnabled: FishUI.Theme.blurEnabled

        MenuItem {
            text: qsTr("Open")
            onTriggered: launcherModel.launch(model.appId)
        }

        MenuItem {
            id: sendToDock
            text: qsTr("Send to dock")
            onTriggered: launcher.sendToDock(model.appId)
        }

        MenuItem {
            id: sendToDesktop
            text: qsTr("Send to desktop")
            onTriggered: launcherModel.sendToDesktop(model.appId)
        }

        MenuItem {
            id: removeFromDock
            text: qsTr("Remove from dock")
            onTriggered: launcher.removeFromDock(model.appId)
        }

        MenuItem {
            id: uninstallItem
            text: qsTr("Uninstall")
            onTriggered: {
                root.uninstallDialog.desktopPath = model.appId
                root.uninstallDialog.appName = model.name
                root.uninstallDialog.visible = true
            }
        }

        function updateActions() {
            sendToDock.visible = launcher.dockAvailable() && !launcher.isPinedDock(model.appId)
            removeFromDock.visible = launcher.dockAvailable() && launcher.isPinedDock(model.appId)
        }
    }

    // Only the icon is clickable; the label and the gaps fall through to the
    // page's dismiss handler.
    MouseArea {
        id: iconMouseArea

        x: icon.x
        y: icon.y
        width: icon.width
        height: icon.height

        acceptedButtons: Qt.LeftButton | Qt.RightButton

        // In-scene drag. Drag.Automatic would hand it to the compositor, which
        // needs a data-device grab our shell surface never gets on Wayland.
        drag.target: control.searchMode || !control.dragLayer ? null
                                                             : control.dragLayer.dragProxy
        drag.axis: Drag.XAndYAxis
        drag.smoothed: false

        onPressed: {
            if (control.searchMode || !control.dragLayer)
                return

            // Sit the proxy on the icon so it does not jump when the drag starts.
            var proxy = control.dragLayer.dragProxy
            var pos = control.mapToItem(control.dragLayer, icon.x, icon.y)
            proxy.iconName = model.iconName
            proxy.sourceItem = control
            proxy.x = pos.x
            proxy.y = pos.y
        }

        drag.onActiveChanged: {
            if (!control.dragLayer)
                return

            if (!iconMouseArea.drag.active)
                control.dragLayer.dragProxy.finish()
            else
                control.dragLayer.dragProxy.dragging = true
        }

        onCanceled: {
            if (control.dragLayer)
                control.dragLayer.dragProxy.abort()
        }

        onClicked: function (mouse) {
            if (mouse.button === Qt.LeftButton) {
                launcherModel.launch(model.appId)
            } else if (mouse.button === Qt.RightButton) {
                uninstallItem.visible = appManager.isCutefishOS()
                _itemMenu.updateActions()
                _itemMenu.popup()
            }
        }
    }

    Rectangle {
        id: newInstallPoint

        width: Math.max(5, Math.round(control.iconSize * 0.06))
        height: width
        radius: height / 2

        visible: !control.dragging && model.newInstalled

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: label.top
        anchors.bottomMargin: Math.round(control.spacing / 3)

        color: FishUI.Theme.highlightColor
    }

    Label {
        id: label

        x: Math.round((control.width - width) / 2)
        y: icon.y + icon.height + control.spacing

        width: control.width - control.spacing * 2
        height: control.labelHeight

        visible: !control.dragging

        text: model.name
        elide: Text.ElideRight
        textFormat: Text.PlainText
        maximumLineCount: 2
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
        font: Qt.font({ family: Qt.application.font.family,
                        pixelSize: control.labelPixelSize })
        color: "white"
    }
}
