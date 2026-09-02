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
import Cutefish.Dock 1.0
import Cutefish.Shell 1.0
import FishUI 1.0 as FishUI

DockItem {
    id: appItem

    property var windowCount: model.windowCount
    property var windowIds: model.windowIds
    property var dragSource: null

    iconName: model.iconName ? model.iconName : "application-x-desktop"
    isActive: model.isActive
    popupText: model.visibleName
    enableActivateDot: windowCount !== 0
    draggable: !model.fixed
    dragItemIndex: index

    function updateMinimizedGeometry() {
        minimizedGeometryTimer.restart()
    }

    onWindowIdsChanged: updateMinimizedGeometry()

    Component.onCompleted: updateMinimizedGeometry()

    Connections {
        target: icon

        function onXChanged() {
            appItem.updateMinimizedGeometry()
        }

        function onYChanged() {
            appItem.updateMinimizedGeometry()
        }

        function onWidthChanged() {
            appItem.updateMinimizedGeometry()
        }

        function onHeightChanged() {
            appItem.updateMinimizedGeometry()
        }
    }

    Connections {
        target: dock

        function onMinimizedGeometrySurfaceChanged() {
            appItem.updateMinimizedGeometry()
        }
    }

    Timer {
        id: minimizedGeometryTimer
        interval: 0
        repeat: false
        onTriggered: dock.updateIconGeometry(icon, windowIds)
    }

    onRightClicked: if (model.appId !== "cutefish-launcher") popupMenu(contextMenu, mouse)

    onClicked: {
        if (mouse.button === Qt.LeftButton)
            appModel.clicked(model.appId)
        else if (mouse.button === Qt.MiddleButton)
            appModel.openNewInstance(model.appId)
    }

    dropArea.onEntered: {
        appItem.dragSource = drag.source
        dropTimer.restart()
    }

    dropArea.onExited: {
        appItem.dragSource = null
        dropTimer.stop()
    }

    dropArea.onDropped: {
        appModel.save()
    }

    Timer {
        id: dropTimer
        interval: 300
        onTriggered: {
            if (appItem.dragSource)
                appModel.move(appItem.dragSource.dragItemIndex,
                              appItem.dragItemIndex)
            else
                appModel.raiseWindow(model.appId)
        }
    }

    ScreenHelper {
        id: _screenHelper
    }

    FishUI.DesktopMenu {
        id: contextMenu
        availableGeometry: _screenHelper.availableScreenRect
        blurEnabled: FishUI.Theme.blurEnabled

        MenuItem {
            text: qsTr("Open")
            visible: windowCount === 0
            onTriggered: appModel.openNewInstance(model.appId)
        }

        MenuItem {
            text: model.visibleName
            visible: windowCount > 0 && model.visibleName
            onTriggered: appModel.openNewInstance(model.appId)
        }

        MenuItem {
            text: model.isPinned ? qsTr("Unpin") : qsTr("Pin")
            visible: model.desktopFile !== ""
            onTriggered: {
                model.isPinned ? appModel.unPin(model.appId) : appModel.pin(model.appId)
            }
        }

        MenuItem {
            visible: windowCount !== 0
            text: windowCount === 1 ? qsTr("Close window")
                                    : qsTr("Close %1 windows").arg(windowCount)
            onTriggered: appModel.closeAllByAppId(model.appId)
        }
    }
}
