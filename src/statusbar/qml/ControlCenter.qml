/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * HarmonyOS 5 Laptop Control Center Exact UX Implementation
 */

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import Qt5Compat.GraphicalEffects

import Cutefish.Accounts 1.0 as Accounts
import Cutefish.Bluetooth 1.0 as Bluetooth
import Cutefish.Network 1.0 as NM
import Cutefish.Appearance 1.0 as FrameworkAppearance
import Cutefish.StatusBar 1.0
import Cutefish.Shell 1.0
import Cutefish.Audio 1.0
import FishUI 1.0 as FishUI

PopupWindow {
    id: control

    width: 360
    height: _mainLayout.implicitHeight + 28

    property var margin: 6
    property point position: Qt.point(0, 0)
    property var defaultSink: paSinkModel.defaultSink

    property bool bluetoothDisConnected: Bluetooth.Manager.bluetoothBlocked
    property var defaultSinkValue: defaultSink ? defaultSink.volume / PulseAudio.NormalVolume * 100.0 : -1

    property bool _airdropActive: false
    property bool _nightLightActive: false
    property bool _locationActive: true
    property bool _powerSaverActive: false

    property var cardBgColor: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.14)
                                                    : Qt.rgba(255, 255, 255, 0.75)
    property var cardBorderColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.16)
                                                        : Qt.rgba(255, 255, 255, 0.60)
    property var subTextColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.70)
                                                     : Qt.rgba(0, 0, 0, 0.60)
    property string iconDir: FishUI.Theme.darkMode ? "dark/" : "light/"

    property var volumeIconName: {
        if (!defaultSink || defaultSink.muted || defaultSinkValue <= 0)
            return "audio-volume-muted-symbolic"
        else if (defaultSinkValue <= 25)
            return "audio-volume-low-symbolic"
        else if (defaultSinkValue <= 75)
            return "audio-volume-medium-symbolic"
        else
            return "audio-volume-high-symbolic"
    }

    onWidthChanged: adjustCorrectLocation()
    onHeightChanged: adjustCorrectLocation()
    onPositionChanged: adjustCorrectLocation()

    color: "transparent"

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    FrameworkAppearance.Appearance { id: appearance }
    Notifications { id: notifications }
    PowerActions { id: powerActions }
    SinkModel { id: paSinkModel }
    Brightness { id: brightness }
    Accounts.UserAccount { id: currentUser }
    ContinuityManager { id: continuity }
    ProcessProvider { id: process }
    Bluetooth.BluetoothManager { id: ccBtManager }
    Bluetooth.DevicesModel { id: ccBtDevicesModel }
    Bluetooth.DevicesProxyModel { id: ccBtProxyModel; sourceModel: ccBtDevicesModel }
    NM.NetworkModel { id: ccNetworkModel }
    NM.AppletProxyModel { id: ccWifiProxyModel; type: NM.AppletProxyModel.WirelessType; sourceModel: ccNetworkModel }

    Timer {
        id: screenshotTimer
        interval: 220
        repeat: false
        onTriggered: process.startDetached("cutefish-screenshot", ["-a"])
    }

    function triggerScreenshot() {
        control.close()
        screenshotTimer.restart()
    }

    function toggleBluetooth() {
        const enable = !control.bluetoothDisConnected
        Bluetooth.Manager.bluetoothBlocked = enable
        for (var i = 0; i < Bluetooth.Manager.adapters.length; ++i) {
            var adapter = Bluetooth.Manager.adapters[i]
            adapter.powered = enable
        }
    }

    function adjustCorrectLocation() {
        var posX = control.position.x
        var posY = control.position.y
        if (posX + control.width >= StatusBar.screenRect.x + StatusBar.screenRect.width)
            posX = StatusBar.screenRect.x + StatusBar.screenRect.width - control.width - control.margin
        posY = rootItem.y + rootItem.height + control.margin
        control.x = Math.round(posX)
        control.y = Math.round(posY)
    }

    FishUI.WindowBlur {
        view: control
        windowRadius: _background.radius
        enabled: FishUI.Theme.blurEnabled
    }

    FishUI.WindowShadow {
        view: control
        radius: _background.radius
        strength: 0.60
    }

    // High Translucency Frosted Glass Panel
    Rectangle {
        id: _background
        anchors.fill: parent
        radius: 24
        color: FishUI.Theme.darkMode ? Qt.rgba(0.12, 0.12, 0.16, 0.65)
                                     : Qt.rgba(0.96, 0.96, 0.98, 0.70)
        antialiasing: true
        border.width: 1 / FishUI.Dpi.ratio
        border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.22)
                                            : Qt.rgba(255, 255, 255, 0.85)

        // Inner Specular Glass Rim
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 23
            color: "transparent"
            border.width: 1 / FishUI.Dpi.ratio
            border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.12)
                                                : Qt.rgba(255, 255, 255, 0.50)
        }
    }

    ColumnLayout {
        id: _mainLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // ==========================================
        // 1. HEADER ROW: Control Center Title + Settings Button
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Control Center")
                font.bold: true
                font.pointSize: 13
                renderType: Text.NativeRendering
                color: FishUI.Theme.textColor
                Layout.fillWidth: true
            }

            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: setMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0.12))
                                        : (setMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.06)) : "transparent")
                antialiasing: true

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    sourceSize: Qt.size(32, 32)
                    source: "qrc:/statusbar/images/" + control.iconDir + "settings.svg"
                }

                MouseArea {
                    id: setMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        control.close()
                        process.startDetached("cutefish-settings")
                    }
                }
            }
        }

        // ==========================================
        // 2. TOP SECTION: 2x2 Grid (Wi-Fi, BT, Wireless Share, Super Device)
        // ==========================================
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 12
            columnSpacing: 12

            // 1. Wi-Fi Card
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: enabledConnections.wirelessEnabled ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (enabledConnections.wirelessEnabled ? "dark/" : control.iconDir) + "network-wireless-connected-100.svg"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: nmHandler.enableWireless(!enabledConnections.wirelessEnabled)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: qsTr("Wi-Fi")
                            font.bold: true
                            font.pointSize: 10
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                        }

                        Label {
                            text: activeConnection.wirelessName ? activeConnection.wirelessName
                                                                : (enabledConnections.wirelessEnabled ? qsTr("On") : qsTr("Off"))
                            font.pointSize: 8.5
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 2. Bluetooth Card
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: !control.bluetoothDisConnected ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (!control.bluetoothDisConnected ? "dark/" : control.iconDir) + "bluetooth-symbolic.svg"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: control.toggleBluetooth()
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: qsTr("Bluetooth")
                            font.bold: true
                            font.pointSize: 10
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                        }

                        Label {
                            text: !control.bluetoothDisConnected ? qsTr("On") : qsTr("Off")
                            font.pointSize: 8.5
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 3. Wireless Share Card
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: control._airdropActive ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (control._airdropActive ? "dark/" : control.iconDir) + "wireless-share.svg"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: control._airdropActive = !control._airdropActive
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: qsTr("Wireless Share")
                            font.bold: true
                            font.pointSize: 10
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                        }

                        Label {
                            text: control._airdropActive ? qsTr("On") : qsTr("Off")
                            font.pointSize: 8.5
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 4. Super Device Card
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 44
                        height: 44
                        radius: 22
                        color: continuity.isConnected ? "#34C759" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (continuity.isConnected ? "dark/" : control.iconDir) + "phone.svg"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                control.close()
                                process.startDetached("cutefish-settings", ["-m", "phonelink"])
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: qsTr("Super Device")
                            font.bold: true
                            font.pointSize: 10
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                        }

                        Label {
                            text: continuity.isConnected ? (continuity.deviceName.length > 0 ? continuity.deviceName : qsTr("Not Connected")) : qsTr("Not Connected")
                            font.pointSize: 8.5
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        // ==========================================
        // 3. MPRIS MEDIA PLAYER CARD
        // ==========================================
        MprisItem {
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            cardBgColor: control.cardBgColor
            cardBorderColor: control.cardBorderColor
        }

        // ==========================================
        // 4. HARMONYOS STACKED SLIDERS (Two separate unified cards)
        // ==========================================
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            Layout.topMargin: 4

            // 1. Brightness Capsule Slider Card
            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: 19
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                // Active Filled Track
                Rectangle {
                    width: Math.max(38, parent.width * (brightnessSlider.value / 100.0))
                    height: parent.height
                    radius: 19
                    color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.20) : Qt.rgba(0, 0, 0, 0.12)
                    antialiasing: true
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Image {
                        width: 20
                        height: 20
                        sourceSize: Qt.size(40, 40)
                        source: "qrc:/statusbar/images/" + control.iconDir + "brightness.svg"
                        z: 2
                    }

                    Slider {
                        id: brightnessSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: brightness.brightness
                        onMoved: brightness.setBrightness(value)
                        z: 3

                        background: Item {} // Transparent track
                        handle: Rectangle {
                            x: brightnessSlider.leftPadding + brightnessSlider.visualPosition * (brightnessSlider.availableWidth - width)
                            y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                            width: 28
                            height: 28
                            radius: 14
                            color: "#FFFFFF"
                            border.width: 1
                            border.color: Qt.rgba(0, 0, 0, 0.15)
                            FishUI.WindowShadow {
                                view: parent
                                radius: 14
                            }
                        }
                    }
                }
            }

            // 2. Volume Capsule Slider Card
            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: 19
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                // Active Filled Track
                Rectangle {
                    width: Math.max(38, parent.width * (Math.max(0, defaultSinkValue) / 100.0))
                    height: parent.height
                    radius: 19
                    color: "#007AFF"
                    antialiasing: true
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Image {
                        width: 20
                        height: 20
                        sourceSize: Qt.size(40, 40)
                        source: "qrc:/statusbar/images/" + (defaultSinkValue > 50 ? "light/" : control.iconDir) + control.volumeIconName + ".svg"
                        z: 2
                    }

                    Slider {
                        id: volumeSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: defaultSinkValue
                        onMoved: {
                            if (defaultSink)
                                defaultSink.volume = value * PulseAudio.NormalVolume / 100.0
                        }
                        z: 3

                        background: Item {} // Transparent track
                        handle: Rectangle {
                            x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                            y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                            width: 28
                            height: 28
                            radius: 14
                            color: "#FFFFFF"
                            border.width: 1
                            border.color: Qt.rgba(0, 0, 0, 0.15)
                            FishUI.WindowShadow {
                                view: parent
                                radius: 14
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // 5. ROUND 5-CIRCLE QUICK TOGGLES ROW
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 8

            // 1. Focus / Do Not Disturb
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: notifications.doNotDisturb ? "#5856D6" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (notifications.doNotDisturb ? "dark/" : control.iconDir) + "do-not-disturb.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: notifications.doNotDisturb = !notifications.doNotDisturb
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Focus")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // 2. Location
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: control._locationActive ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (control._locationActive ? "dark/" : control.iconDir) + "location-mode.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: control._locationActive = !control._locationActive
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Location")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // 3. Airplane Mode
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: control._airdropActive ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (control._airdropActive ? "dark/" : control.iconDir) + "airplane-mode.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: control._airdropActive = !control._airdropActive
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Airplane")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // 4. Dark Mode
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: FishUI.Theme.darkMode ? "#007AFF" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (FishUI.Theme.darkMode ? "dark/" : control.iconDir) + "dark-mode.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: appearance.switchDarkMode(!FishUI.Theme.darkMode)
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Dark Mode")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // 5. Display / Projection
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08)
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + control.iconDir + "projection-mode.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            control.close()
                            process.startDetached("cutefish-settings", ["-m", "display"])
                        }
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Projection")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }
        }

        // ==========================================
        // 6. BOTTOM ROW: Power Saver, Night Shift, Input Share
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Power Saver
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: control._powerSaverActive ? "#34C759" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (control._powerSaverActive ? "dark/" : control.iconDir) + "power-mode.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: control._powerSaverActive = !control._powerSaverActive
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Power Saver")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // Night Shift
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: control._nightLightActive ? "#FF9500" : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + (control._nightLightActive ? "dark/" : control.iconDir) + "eye-protection.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: control._nightLightActive = !control._nightLightActive
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Night Shift")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // Input Share
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 48
                    radius: 24
                    color: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08)
                    antialiasing: true

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/statusbar/images/" + control.iconDir + "input-share.svg"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            control.close()
                            process.startDetached("cutefish-settings", ["-m", "touchpad"])
                        }
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Input Share")
                    font.pointSize: 8
                    renderType: Text.NativeRendering
                    color: FishUI.Theme.textColor
                }
            }

            // Empty Space Padders to align left
            Item { Layout.fillWidth: true }
            Item { Layout.fillWidth: true }
        }
    }
}
