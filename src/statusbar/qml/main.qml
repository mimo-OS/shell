/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     Reion Wong <aj@cutefishos.com>
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
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects

import Cutefish.Accounts 1.0 as Accounts
import Cutefish.System 1.0 as System
import Cutefish.Shell 1.0
import Cutefish.StatusBar 1.0
import Cutefish.NetworkManagement 1.0 as NM
import FishUI 1.0 as FishUI

Item {
    id: rootItem

    property int iconSize: 16

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    property bool darkMode: false
    property color textColor: rootItem.darkMode ? "#FFFFFF" : "#000000";
    property var fontSize: rootItem.height ? rootItem.height / 3 : 1

    property var timeFormat: StatusBar.twentyFourTime ? "ddd MMM d HH:mm" : "ddd MMM d h:mmAP"

    onTimeFormatChanged: {
        timeTimer.restart()
    }

    System.Wallpaper {
        id: sysWallpaper

        function reload() {
            if (sysWallpaper.type === 0)
                bgHelper.setBackgound(sysWallpaper.path)
            else
                bgHelper.setColor(sysWallpaper.color)
        }

        Component.onCompleted: sysWallpaper.reload()

        onTypeChanged: sysWallpaper.reload()
        onColorChanged: sysWallpaper.reload()
        onPathChanged: sysWallpaper.reload()
    }

    Accounts.UserAccount {
        id: currentUser
    }

    BackgroundHelper {
        id: bgHelper

        onNewColor: function (color, darkMode) {
            background.color = color
            rootItem.darkMode = darkMode
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        opacity: 0.3

//        color: FishUI.Theme.darkMode ? "#4D4D4D" : "#FFFFFF"

//        Behavior on color {
//            ColorAnimation {
//                duration: 100
//                easing.type: Easing.Linear
//            }
//        }
    }

    FishUI.PopupTips {
        id: popupTips
    }

    // Wayland tells a client nothing about the panels, so the menus are given
    // the work area the shell itself reserves.
    ScreenHelper {
        id: screenHelper
    }

    // Application menu
    //
    // The menu bar behaves the way macOS does: the pointer switches between
    // the menus while one of them is open, and so do the arrow keys. Both
    // reuse a single popup window - closing and reopening one would hand the
    // keyboard grab back and forth and lose the menu the user is in.
    //
    // The switch is driven by the menu bar's own hover. The open menu sees the
    // pointer too, but only as coordinates in whatever window the compositor
    // handed the event to, and on Wayland a client does not know where its
    // windows are: the dock reports its own top left as the origin, so the
    // pointer crossing the dock used to read as a walk along the menu bar.
    property int appMenuIndex: -1

    function openAppMenu(index, keyboard) {
        if (index < 0 || index >= appMenuView.count)
            return

        var item = appMenuView.itemAtIndex(index)
        if (!item)
            return

        var menuObject = appMenuModel.menuAt(index)

        // A menu bar entry can be a plain action instead of a menu.
        if (!menuObject) {
            rootItem.closeAppMenu()
            appMenuModel.trigger(index)
            return
        }

        appMenuPopup.closeSubmenus()
        appMenuPopup.menuModel = null
        appMenuPopupModel.menu = menuObject
        appMenuPopup.menuModel = appMenuPopupModel
        appMenuPopup.setCurrentIndex(-1)
        appMenuPopup.keyboardNavigation = !!keyboard

        rootItem.appMenuIndex = index

        rootItem.popupMenuAt(appMenuPopup, item)
    }

    function closeAppMenu() {
        rootItem.appMenuIndex = -1

        if (appMenuPopup.visible)
            appMenuPopup.dismissAllPopups()
    }

    function popupMenuAt(menu, item) {
        menu.transientParent = rootItem.Window.window
        var pos = item.mapToGlobal(0, item.height + FishUI.Units.smallSpacing)
        menu.popupAt(pos.x, pos.y)
    }

    function openActicityMenu() {
        if (!acticity.title)
            return
        rootItem.popupMenuAt(acticityMenu, acticityItem)
    }

    // Left and right walk to the neighbouring menu, wrapping around.
    function stepAppMenu(step) {
        var count = appMenuView.count
        if (count === 0)
            return

        rootItem.openAppMenu((rootItem.appMenuIndex + step + count) % count, true)
    }

    QMenuModel {
        id: appMenuPopupModel
    }

    QMenuPopup {
        id: appMenuPopup

        availableGeometry: screenHelper.availableScreenRect

        onKeyNavigationLeft: rootItem.stepAppMenu(-1)
        onKeyNavigationRight: rootItem.stepAppMenu(1)

        onVisibleChanged: {
            if (!visible)
                rootItem.appMenuIndex = -1
        }
    }

    FishUI.DesktopMenu {
        id: acticityMenu

        availableGeometry: screenHelper.availableScreenRect

        MenuItem {
            text: qsTr("Close")
            onTriggered: acticity.close()
        }
    }

    // Main layout
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: FishUI.Units.largeSpacing
        anchors.rightMargin: FishUI.Units.largeSpacing
        spacing: FishUI.Units.largeSpacing

        // User menu
        StandardItem {
            id: userItem

            checked: userMenu.item ? userMenu.item.visible : false
            Layout.fillHeight: true
            Layout.preferredWidth: rootItem.iconSize + FishUI.Units.smallSpacing * 2

            onClicked: {
                if (!userMenu.item)
                    return

                if (userMenu.item.visible) {
                    userMenu.item.dismissAllPopups()
                } else {
                    var pos = mapToGlobal(0, height + FishUI.Units.smallSpacing)
                    userMenu.item.showAt(pos.x, pos.y)
                }
            }

            Image {
                id: userIcon
                anchors.centerIn: parent
                width: rootItem.iconSize
                height: width
                sourceSize: String(source) === "image://icontheme/default-user" ? Qt.size(width, height) : undefined
                source: currentUser.iconFileName ? "file:///" + currentUser.iconFileName : "image://icontheme/default-user"
                antialiasing: true
                smooth: false

                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Item {
                        width: userIcon.width
                        height: userIcon.height

                        Rectangle {
                            anchors.fill: parent
                            radius: parent.height / 2
                        }
                    }
                }
            }
        }

        // App name
        StandardItem {
            id: acticityItem
            visible: !!acticity.title
            Layout.fillHeight: true
            Layout.preferredWidth: Math.min(rootItem.width / 3,
                                            acticityLayout.implicitWidth + FishUI.Units.largeSpacing)
            onClicked: rootItem.openActicityMenu()

            Connections {
                target: acticityItem.mouseArea

                function onDoubleClicked() {
                    rootItem.openActicityMenu()
                }
            }

            RowLayout {
                id: acticityLayout
                anchors.fill: parent
                anchors.leftMargin: FishUI.Units.smallSpacing
                anchors.rightMargin: FishUI.Units.smallSpacing
                spacing: FishUI.Units.smallSpacing

                Label {
                    id: acticityLabel
                    text: acticity.title
                    Layout.fillWidth: true
                    elide: Qt.ElideRight
                    color: rootItem.textColor
                    visible: text
                    Layout.alignment: Qt.AlignVCenter
                    font.pointSize: rootItem.fontSize
                }
            }
        }

        // App menu
        Item {
            id: appMenuItem
            Layout.fillHeight: true
            Layout.fillWidth: true

            ListView {
                id: appMenuView
                anchors.fill: parent
                orientation: Qt.Horizontal
                spacing: FishUI.Units.smallSpacing
                visible: appMenuModel.visible
                interactive: false
                clip: true

                model: appMenuModel

                onVisibleChanged: {
                    if (!visible)
                        rootItem.closeAppMenu()
                }

                delegate: StandardItem {
                    id: _menuItem
                    width: _actionText.width + FishUI.Units.largeSpacing
                    height: ListView.view.height
                    checked: rootItem.appMenuIndex === index

                    Text {
                        id: _actionText
                        anchors.centerIn: parent
                        color: rootItem.textColor
                        font.pointSize: rootItem.fontSize
                        text: {
                            var text = activeMenu
                            text = text.replace(/([^&]*)&(.)([^&]*)/g, function (match, p1, p2, p3) {
                                return p1.concat(p2, p3)
                            })
                            return text
                        }
                    }

                    // A menu opens on click or double-click. While one is
                    // open, moving onto another entry switches to it: whether
                    // the pointer event reaches the menu bar or the menu
                    // depends on how the compositor routes the popup grab,
                    // so both are wired up to the same switch.
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: rootItem.appMenuIndex !== -1
                        onClicked: rootItem.openAppMenu(index)
                        onDoubleClicked: rootItem.openAppMenu(index)
                        onEntered: {
                            if (rootItem.appMenuIndex !== -1 &&
                                    rootItem.appMenuIndex !== index)
                                rootItem.openAppMenu(index)
                        }
                    }
                }

                AppMenuModel {
                    id: appMenuModel

                    // An application can ask for one of its menus to be
                    // opened, which is how a menu shortcut arrives here.
                    onRequestActivateIndex: function (index) {
                        rootItem.openAppMenu(index, true)
                    }
                }
            }
        }

        // System tray(Right)
        SystemTray {}

        StandardItem {
            id: controler

            checked: controlCenter.item.visible
            Layout.fillHeight: true
            Layout.preferredWidth: _controlerLayout.implicitWidth + FishUI.Units.largeSpacing

            onClicked: {
                toggleDialog()
            }

            function toggleDialog() {
                if (controlCenter.item.visible)
                    controlCenter.item.close()
                else {
                    // 先初始化，用户可能会通过Alt鼠标左键移动位置
                    controlCenter.item.position = Qt.point(0, 0)
                    controlCenter.item.position = mapToGlobal(0, 0)
                    controlCenter.item.open()
                }
            }

            RowLayout {
                id: _controlerLayout
                anchors.fill: parent
                anchors.leftMargin: FishUI.Units.smallSpacing
                anchors.rightMargin: FishUI.Units.smallSpacing

                spacing: FishUI.Units.largeSpacing

                Image {
                    id: volumeIcon
                    visible: controlCenter.item.defaultSink
                    source: "qrc:/statusbar/images/" + (rootItem.darkMode ? "dark/" : "light/") + controlCenter.item.volumeIconName + ".svg"
                    width: rootItem.iconSize
                    height: width
                    sourceSize: Qt.size(width, height)
                    asynchronous: true
                    Layout.alignment: Qt.AlignCenter
                    antialiasing: true
                    smooth: false
                }

                Image {
                    id: wirelessIcon
                    width: rootItem.iconSize
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: activeConnection.wirelessIcon ? "qrc:/statusbar/images/" + (rootItem.darkMode ? "dark/" : "light/") + activeConnection.wirelessIcon + ".svg" : ""
                    asynchronous: true
                    Layout.alignment: Qt.AlignCenter
                    visible: enabledConnections.wirelessHwEnabled &&
                             enabledConnections.wirelessEnabled &&
                             activeConnection.wirelessName &&
                             wirelessIcon.status === Image.Ready
                    antialiasing: true
                    smooth: false
                }

                // Battery Item
                RowLayout {
                    visible: battery.available

                    Image {
                        id: batteryIcon
                        height: rootItem.iconSize
                        width: height + 6
                        sourceSize: Qt.size(width, height)
                        source: "qrc:/statusbar/images/" + (rootItem.darkMode ? "dark/" : "light/") + battery.iconSource
                        Layout.alignment: Qt.AlignCenter
                        antialiasing: true
                        smooth: false
                    }

                    Label {
                        text: battery.chargePercent + "%"
                        font.pointSize: rootItem.fontSize
                        color: rootItem.textColor
                        visible: battery.showPercentage
                    }
                }
            }
        }

        // Pop-up notification center and calendar
        StandardItem {
            id: datetimeItem

            checked: notificationCenter.item ? notificationCenter.item.visible : false
            Layout.fillHeight: true
            Layout.preferredWidth: _dateTimeLayout.implicitWidth + FishUI.Units.smallSpacing

            onClicked: {
                toggleDialog()
            }

            function toggleDialog() {
                if (!notificationCenter.item)
                    return

                if (notificationCenter.item.visible)
                    notificationCenter.item.close()
                else {
                    notificationCenter.item.position = Qt.point(0, 0)
                    notificationCenter.item.position = mapToGlobal(0, 0)
                    notificationCenter.item.open()
                }
            }

            RowLayout {
                id: _dateTimeLayout
                anchors.fill: parent

//                Image {
//                    width: rootItem.iconSize
//                    height: width
//                    sourceSize: Qt.size(width, height)
//                    source: "qrc:/statusbar/images/" + (rootItem.darkMode ? "dark/" : "light/") + "notification-symbolic.svg"
//                    asynchronous: true
//                    Layout.alignment: Qt.AlignCenter
//                    antialiasing: true
//                    smooth: false
//                }

                Label {
                    id: timeLabel
                    Layout.alignment: Qt.AlignCenter
                    font.pointSize: rootItem.fontSize
                    color: rootItem.textColor

                    Timer {
                        id: timeTimer
                        interval: 1000
                        repeat: true
                        running: true
                        triggeredOnStart: true
                        onTriggered: {
                            timeLabel.text = new Date().toLocaleString(Qt.locale(), rootItem.timeFormat)
                        }
                    }
                }
            }
        }

    }

    MouseArea {
        id: _sliding
        anchors.fill: parent
        z: -1

        property int startY: -1
        property bool activated: false

        onActivatedChanged: {
            // TODO
            // if (activated)
            //     acticity.move()
        }

        onPressed: {
            startY = mouse.y
        }

        onReleased: {
            startY = -1
        }

        onDoubleClicked: {
            acticity.toggleMaximize()
        }

        onMouseYChanged: {
            if (startY === parseInt(mouse.y)) {
                activated = false
                return
            }

            // Up
            if (startY > parseInt(mouse.y)) {
                activated = false
                return
            }

            if (mouse.y > rootItem.height)
                activated = true
            else
                activated = false
        }
    }

    // Components
    Loader {
        id: controlCenter
        sourceComponent: ControlCenter {}
        asynchronous: true
    }

    // The notification centre lives in the notification component's resources,
    // so it comes in by URL rather than as an inline component.
    Loader {
        id: notificationCenter
        source: "qrc:/notification/qml/NotificationCenter.qml"
        asynchronous: true

        // Anchor the popup to the status bar. Without a transient parent Qt
        // has nothing to make an xdg_popup out of and falls back to an
        // ordinary toplevel, which KWin then decorates and places itself -
        // exactly what the notification centre used to look like when it lived
        // in its own process.
        onLoaded: item.transientParent = rootItem.Window.window
    }

    // `cutefish-notificationd -s` used to pop the window up from another
    // process; now com.cutefish.Notification.showWindow() lands here.
    Connections {
        target: notificationManager

        function onWindowRequested() {
            datetimeItem.toggleDialog()
        }
    }

    Loader {
        id: userMenu
        sourceComponent: UserMenu {}
        asynchronous: true

        onLoaded: {
            item.transientParent = rootItem.Window.window
            item.availableGeometry = Qt.binding(function () {
                return screenHelper.availableScreenRect
            })
        }
    }

    NM.ActiveConnection {
        id: activeConnection
    }

    NM.EnabledConnections {
        id: enabledConnections
    }

    NM.Handler {
        id: nmHandler
    }
}
