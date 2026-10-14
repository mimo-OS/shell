/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * HarmonyOS-inspired Control Center UI refinement
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
    height: _mainLayout.implicitHeight + 32

    property var margin: 6
    property point position: Qt.point(0, 0)
    property var defaultSink: paSinkModel.defaultSink

    property bool bluetoothDisConnected: Bluetooth.Manager.bluetoothBlocked
    property var defaultSinkValue: defaultSink ? defaultSink.volume / PulseAudio.NormalVolume * 100.0 : -1

    property bool _airdropActive: false
    property bool _nightLightActive: false
    property bool _locationActive: true
    property bool _powerSaverActive: false
    property bool _keyboardBacklightActive: false
    property bool _wifiActive: true
    property int _powerModeState: battery.powerMode
    property int _keyboardBacklightLevel: 0

    property color cardBgColor: FishUI.Theme.darkMode ? Qt.rgba(0.18, 0.20, 0.26, 0.68)
                                                      : Qt.rgba(1, 1, 1, 0.68)
    property color cardBorderColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.14)
                                                          : Qt.rgba(1, 1, 1, 0.85)
    property color inactiveCircleColor: FishUI.Theme.darkMode ? Qt.rgba(0.24, 0.26, 0.32, 0.65)
                                                              : "#FFFFFF"
    property color inactiveCircleBorderColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10)
                                                                    : Qt.rgba(0, 0, 0, 0.08)
    property string inactiveIconDir: FishUI.Theme.darkMode ? "dark/" : "light/"
    property color subTextColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.70)
                                                       : Qt.rgba(0.12, 0.14, 0.20, 0.62)
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
        onTriggered: process.startDetached("cutefish-screenshot")
    }

    function triggerScreenshot() {
        control.close()
        screenshotTimer.restart()
    }

    function cyclePowerMode() {
        var nextMode = (battery.powerMode + 1) % 3
        battery.setPowerMode(nextMode)
        control._powerModeState = nextMode

        var profile = "balanced"
        var sys76 = "balanced"
        var cpu = "performance"
        var tuned = "balanced"

        if (nextMode === 1) {
            profile = "performance"
            sys76 = "performance"
            cpu = "performance"
            tuned = "throughput-performance"
        } else if (nextMode === 2) {
            profile = "power-saver"
            sys76 = "battery"
            cpu = "powersave"
            tuned = "powersave"
        }

        process.startDetached("dbus-send", [
            "--system",
            "--print-reply",
            "--dest=org.freedesktop.UPower.PowerProfiles",
            "/org/freedesktop/UPower/PowerProfiles",
            "org.freedesktop.DBus.Properties.Set",
            "string:org.freedesktop.UPower.PowerProfiles",
            "string:ActiveProfile",
            "variant:string:" + profile
        ])
        process.startDetached("powerprofilesctl", ["set", profile])
        process.startDetached("system76-power", ["profile", sys76])
        process.startDetached("cpupower", ["frequency-set", "-g", cpu])
        process.startDetached("auto-cpufreq", ["--force=" + (nextMode === 1 ? "performance" : (nextMode === 2 ? "powersave" : "reset"))])
        process.startDetached("tuned-adm", ["profile", tuned])
    }

    Timer {
        id: nightLightKeepAliveTimer
        interval: 4000
        repeat: true
        running: control._nightLightActive
        triggeredOnStart: true
        onTriggered: {
            process.startDetached("qdbus6", ["org.kde.KWin", "/org/kde/KWin/NightLight", "org.kde.KWin.NightLight.preview", "3500"])
            process.startDetached("qdbus", ["org.kde.KWin", "/org/kde/KWin/NightLight", "org.kde.KWin.NightLight.preview", "3500"])
            process.startDetached("sh", ["-c", "for out in $(xrandr | grep ' connected' | cut -d' ' -f1); do xrandr --output \"$out\" --gamma 1.0:0.82:0.65 2>/dev/null; done"])
        }
    }

    function toggleNightLight() {
        control._nightLightActive = !control._nightLightActive
        if (!control._nightLightActive) {
            process.startDetached("qdbus6", ["org.kde.KWin", "/org/kde/KWin/NightLight", "org.kde.KWin.NightLight.stopPreview"])
            process.startDetached("qdbus", ["org.kde.KWin", "/org/kde/KWin/NightLight", "org.kde.KWin.NightLight.stopPreview"])
            process.startDetached("sh", ["-c", "for out in $(xrandr | grep ' connected' | cut -d' ' -f1); do xrandr --output \"$out\" --gamma 1.0:1.0:1.0 2>/dev/null; done"])
        }
    }

    function toggleKeyboardBacklight() {
        control._keyboardBacklightLevel = (control._keyboardBacklightLevel + 1) % 3
        control._keyboardBacklightActive = (control._keyboardBacklightLevel > 0)

        var valPct = "0%"
        var valStep = "0"
        var valGnome = "0.0"
        var valAsus = "off"

        if (control._keyboardBacklightLevel === 2) {
            valPct = "100%"
            valStep = "2"
            valGnome = "1.0"
            valAsus = "high"
        } else if (control._keyboardBacklightLevel === 1) {
            valPct = "50%"
            valStep = "1"
            valGnome = "0.5"
            valAsus = "low"
        }

        process.startDetached("dbus-send", [
            "--system",
            "--type=method_call",
            "--dest=org.freedesktop.UPower",
            "/org/freedesktop/UPower/KbdBacklight",
            "org.freedesktop.UPower.KbdBacklight.SetBrightness",
            "int32:" + valStep
        ])
        process.startDetached("brightnessctl", ["--device=*kbd_backlight*", "set", valPct])
        process.startDetached("brightnessctl", ["--device=*keyboard*", "set", valPct])
        process.startDetached("brightnessctl", ["--class=leds", "set", valPct])
        process.startDetached("light", ["-s", "sysfs/leds/smc::kbd_backlight", "-S", valPct.replace("%", "")])
        process.startDetached("asusctl", ["-k", valAsus])
        process.startDetached("gsettings", ["set", "org.gnome.settings-daemon.peripherals.keyboard", "backlight-brightness", valGnome])
    }

    function toggleFocusDND() {
        notifications.doNotDisturb = !notifications.doNotDisturb
        if (notifications.doNotDisturb) {
            process.startDetached("dunstctl", ["set-paused", "true"])
        } else {
            process.startDetached("dunstctl", ["set-paused", "false"])
        }
    }

    function toggleWifi() {
        control._wifiActive = !control._wifiActive
        if (typeof nmHandler !== "undefined" && nmHandler) {
            nmHandler.enableWireless(control._wifiActive)
        }
        process.startDetached("nmcli", ["radio", "wifi", control._wifiActive ? "on" : "off"])
    }

    function toggleBluetooth() {
        control.bluetoothDisConnected = !control.bluetoothDisConnected
        if (typeof Bluetooth !== "undefined" && Bluetooth.Manager) {
            Bluetooth.Manager.bluetoothBlocked = control.bluetoothDisConnected
            for (var i = 0; i < Bluetooth.Manager.adapters.length; ++i) {
                var adapter = Bluetooth.Manager.adapters[i]
                adapter.powered = !control.bluetoothDisConnected
            }
        }
        process.startDetached("bluetoothctl", ["power", !control.bluetoothDisConnected ? "on" : "off"])
        process.startDetached("rfkill", [!control.bluetoothDisConnected ? "unblock" : "block", "bluetooth"])
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
        radius: 26
        color: FishUI.Theme.darkMode ? Qt.rgba(0.08, 0.09, 0.13, 0.82)
                                     : Qt.rgba(0.92, 0.94, 0.98, 0.72)
        antialiasing: true
        border.width: 1 / FishUI.Dpi.ratio
        border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.20)
                                            : Qt.rgba(1, 1, 1, 0.95)

        // Inner Specular Glass Rim
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: 25
            color: "transparent"
            border.width: 1 / FishUI.Dpi.ratio
            border.color: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.12)
                                                : Qt.rgba(1, 1, 1, 0.50)
        }
    }

    ColumnLayout {
        id: _mainLayout
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // ==========================================
        // 1. HEADER ROW: Control Center Title + Settings Button
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Control Center")
                font.bold: true
                font.pixelSize: 15
                color: FishUI.Theme.textColor
                Layout.fillWidth: true
            }

            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: lockMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0.12))
                                        : (lockMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.06)) : "transparent")
                antialiasing: true

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    sourceSize: Qt.size(32, 32)
                    source: "qrc:/statusbar/images/" + control.iconDir + "lock.svg"
                }

                MouseArea {
                    id: lockMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        control.close()
                        powerActions.lockScreen()
                    }
                }
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
                        if (!process.startDetached("cutefish-settings")) {
                            if (!process.startDetached("systemsettings")) {
                                process.startDetached("gnome-control-center")
                            }
                        }
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
            rowSpacing: 10
            columnSpacing: 10

            // 1. Wi-Fi Card
            Rectangle {
                id: wifiCard
                Layout.fillWidth: true
                height: 60
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true
                scale: wifiMouse.pressed ? 0.94 : 1.0

                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutBack }
                }

                MouseArea {
                    id: wifiMouse
                    anchors.fill: parent
                    onClicked: control.toggleWifi()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 21
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        Layout.alignment: Qt.AlignVCenter
                        color: control._wifiActive ? "#007AFF" : control.inactiveCircleColor
                        border.width: control._wifiActive ? 0 : (!FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0)
                        border.color: control.inactiveCircleBorderColor
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (control._wifiActive ? "dark/wifi.svg" : (control.inactiveIconDir + "wifi_disabled.svg"))
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Wi-Fi")
                            font.bold: true
                            font.pixelSize: 12
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (typeof activeConnection !== "undefined" && activeConnection && activeConnection.wirelessName)
                                ? activeConnection.wirelessName
                                : (control._wifiActive ? qsTr("On") : qsTr("Off"))
                            font.pixelSize: 11
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 2. Bluetooth Card
            Rectangle {
                id: btCard
                Layout.fillWidth: true
                height: 60
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true
                scale: btMouse.pressed ? 0.94 : 1.0

                Behavior on scale {
                    NumberAnimation { duration: 120; easing.type: Easing.OutBack }
                }

                MouseArea {
                    id: btMouse
                    anchors.fill: parent
                    onClicked: control.toggleBluetooth()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 21
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        Layout.alignment: Qt.AlignVCenter
                        color: !control.bluetoothDisConnected ? "#007AFF" : control.inactiveCircleColor
                        border.width: !control.bluetoothDisConnected ? 0 : (!FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0)
                        border.color: control.inactiveCircleBorderColor
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (!control.bluetoothDisConnected ? "dark/" : control.inactiveIconDir) + (control.bluetoothDisConnected ? "bluetooth-off.svg" : "bluetooth-symbolic.svg")
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Bluetooth")
                            font.bold: true
                            font.pixelSize: 12
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: !control.bluetoothDisConnected ? qsTr("On") : qsTr("Off")
                            font.pixelSize: 11
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 3. Wireless Share Card
            Rectangle {
                Layout.fillWidth: true
                height: 60
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                MouseArea {
                    anchors.fill: parent
                    onClicked: control._airdropActive = !control._airdropActive
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 21
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        Layout.alignment: Qt.AlignVCenter
                        color: control._airdropActive ? "#007AFF" : control.inactiveCircleColor
                        border.width: control._airdropActive ? 0 : (!FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0)
                        border.color: control.inactiveCircleBorderColor
                        antialiasing: true

                        Image {
                            anchors.centerIn: parent
                            width: 22
                            height: 22
                            sourceSize: Qt.size(44, 44)
                            source: "qrc:/statusbar/images/" + (control._airdropActive ? "dark/wireless-share.svg" : (control.inactiveIconDir + "wireless-share-off.svg"))
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Wireless Share")
                            font.bold: true
                            font.pixelSize: 12
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: control._airdropActive ? qsTr("On") : qsTr("Off")
                            font.pixelSize: 11
                            color: FishUI.Theme.disabledTextColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // 4. Super Device Card
            Rectangle {
                Layout.fillWidth: true
                height: 60
                radius: 16
                color: control.cardBgColor
                border.width: 1 / FishUI.Dpi.ratio
                border.color: control.cardBorderColor
                antialiasing: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 10

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 21
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 42
                        Layout.alignment: Qt.AlignVCenter
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
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("KDE Connect")
                            font.bold: true
                            font.pixelSize: 12
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: continuity.isConnected ? (continuity.deviceName.length > 0 ? continuity.deviceName : qsTr("Not Connected")) : qsTr("Not Connected")
                            font.pixelSize: 11
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
        // 4. HARMONYOS STACKED SLIDERS (One unified card holding both)
        // ==========================================

        // Unified HarmonyOS brightness and volume ca
        // Floating brightness and volume sliders
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // Brightness pill
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: FishUI.Theme.darkMode
                        ? Qt.rgba(0.18, 0.20, 0.26, 0.80)
                        : Qt.rgba(0, 0, 0, 0.10)
                }

                Rectangle {
                    width: parent.width
                        * Math.max(0, Math.min(100,
                            brightnessSlider.value)) / 100
                    height: parent.height
                    radius: height / 2
                    color: FishUI.Theme.darkMode
                        ? Qt.rgba(1, 1, 1, 0.92)
                        : "#FFFFFF"
                }

                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    sourceSize: Qt.size(44, 44)
                    source: "qrc:/statusbar/images/light/" + (brightness.value >= 99 ? "brightness_max.svg" : "brightness_min.svg")
                    z: 2
                }

                Slider {
                    id: brightnessSlider
                    anchors.fill: parent
                    from: 0
                    to: 100
                    value: brightness.value
                    leftPadding: 40
                    rightPadding: 0

                    onMoved: brightness.setValue(value)
                    onPressedChanged: {
                        if (!pressed)
                            brightness.setValue(value)
                    }

                    background: Item {}

                    handle: Item {
                        implicitWidth: 0
                        implicitHeight: 0
                    }
                }
            }

            // Volume pill
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: FishUI.Theme.darkMode
                        ? Qt.rgba(0.18, 0.20, 0.26, 0.80)
                        : Qt.rgba(0, 0, 0, 0.10)
                }

                Rectangle {
                    width: parent.width
                        * Math.max(0, Math.min(100,
                            defaultSinkValue)) / 100
                    height: parent.height
                    radius: height / 2
                    color: FishUI.Theme.darkMode
                        ? Qt.rgba(1, 1, 1, 0.92)
                        : "#FFFFFF"
                }

                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 11
                    anchors.verticalCenter: parent.verticalCenter
                    width: 17
                    height: 17
                    sourceSize: Qt.size(34, 34)
                    source: "qrc:/statusbar/images/light/"
                        + control.volumeIconName + ".svg"
                    z: 2
                }

                Slider {
                    id: volumeSlider
                    anchors.fill: parent
                    from: 0
                    to: 100
                    value: Math.max(0, defaultSinkValue)
                    leftPadding: 36
                    rightPadding: 0

                    onMoved: {
                        if (defaultSink)
                            defaultSink.volume =
                                value * PulseAudio.NormalVolume / 100.0
                    }
                    onPressedChanged: {
                        if (!pressed && defaultSink)
                            defaultSink.volume =
                                value * PulseAudio.NormalVolume / 100.0
                    }

                    background: Item {}

                    handle: Item {
                        implicitWidth: 0
                        implicitHeight: 0
                    }
                }
            }
        }



        // ==========================================
        // 5. ROUND QUICK TOGGLES GRID (2 Rows x 5 Items = 10 Toggles)
        // ==========================================

        ColumnLayout {
            id: bottomQuickSettings
            Layout.fillWidth: true
            spacing: 8

            // Row 1: Power Mode, Night, Capture, Focus, Dark (5 items)
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                // 1. Power Mode (3-Way Switch: Balanced -> Performance -> Saver)
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control.cyclePowerMode()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control._powerModeState === 1
                                ? "#FF9500"
                                : (control._powerModeState === 2
                                   ? "#34C759"
                                   : control.inactiveCircleColor)
                            border.width: control._powerModeState === 0 && !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/" + (control._powerModeState !== 0 ? "dark/" : control.inactiveIconDir) + (control._powerModeState === 1 ? "turbo.svg" : (control._powerModeState === 2 ? "power-saver.svg" : "balanced.svg"))
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: control._powerModeState === 1
                                ? qsTr("Perf")
                                : (control._powerModeState === 2
                                   ? qsTr("Saver")
                                   : qsTr("Balanced"))
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 2. Night Shift
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control.toggleNightLight()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control._nightLightActive
                                ? "#FF9500"
                                : control.inactiveCircleColor
                            border.width: !control._nightLightActive && !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + (control._nightLightActive
                                       ? "dark/" : control.inactiveIconDir)
                                    + "eye-protection.svg"
                                scale: control._nightLightActive ? 1.10 : 1.0
                                Behavior on scale {
                                    NumberAnimation { duration: 200; easing.type: Easing.OutBack }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Night")
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 3. Screenshot (Capture)
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control.triggerScreenshot()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control.inactiveCircleColor
                            border.width: !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + control.inactiveIconDir + "screenshot.svg"
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Capture")
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 4. Focus (DND Toggle)
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control.toggleFocusDND()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: notifications.doNotDisturb
                                ? "#5856D6"
                                : control.inactiveCircleColor
                            border.width: !notifications.doNotDisturb && !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + (notifications.doNotDisturb
                                       ? "dark/do-not-disturb.svg" : (control.inactiveIconDir + "do-not-disturb-off.svg"))
                                rotation: notifications.doNotDisturb ? 360 : 0
                                Behavior on rotation {
                                    NumberAnimation { duration: 350; easing.type: Easing.OutBack }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Focus")
                            font.pointSize: 7.5
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 5. Dark Mode
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: appearance.switchDarkMode(!FishUI.Theme.darkMode)
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: FishUI.Theme.darkMode
                                ? "#007AFF"
                                : control.inactiveCircleColor
                            border.width: !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                id: darkModeIcon
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + (FishUI.Theme.darkMode
                                       ? "dark/dark-mode.svg" : "light/light-mode.svg")
                                rotation: FishUI.Theme.darkMode ? 0 : 180
                                Behavior on rotation {
                                    NumberAnimation { duration: 300; easing.type: Easing.OutBack }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: FishUI.Theme.darkMode ? qsTr("Dark") : qsTr("Light")
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // Row 2: Key Light, Location, Logout, Restart, Power Off (5 items)
            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                // 6. Key Light (Keyboard Backlight - 3-level cycle)
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control.toggleKeyboardBacklight()
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control._keyboardBacklightActive
                                ? "#007AFF"
                                : control.inactiveCircleColor
                            border.width: !control._keyboardBacklightActive && !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + (control._keyboardBacklightActive ? "dark/" : control.inactiveIconDir)
                                    + (control._keyboardBacklightLevel === 2
                                       ? "keyboard-backlight-high.svg"
                                       : (control._keyboardBacklightLevel === 1
                                          ? "keyboard-backlight-low.svg"
                                          : "keyboard-backlight-off.svg"))
                                scale: control._keyboardBacklightActive ? 1.10 : 1.0
                                Behavior on scale {
                                    NumberAnimation { duration: 200; easing.type: Easing.OutBack }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: control._keyboardBacklightLevel === 2
                                ? qsTr("Key High")
                                : (control._keyboardBacklightLevel === 1
                                   ? qsTr("Key Low")
                                   : qsTr("Backlight"))
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 7. Location
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: control._locationActive = !control._locationActive
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control._locationActive
                                ? "#007AFF"
                                : control.inactiveCircleColor
                            border.width: !control._locationActive && !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + (control._locationActive
                                       ? "dark/location-mode.svg" : (control.inactiveIconDir + "location-mode-off.svg"))
                                scale: control._locationActive ? 1.10 : 1.0
                                Behavior on scale {
                                    NumberAnimation { duration: 200; easing.type: Easing.OutBack }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Location")
                            font.pointSize: 7.5
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 8. Logout
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            control.close()
                            rootItem.showPowerDialog("logout")
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control.inactiveCircleColor
                            border.width: !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + control.inactiveIconDir + "system-log-out.svg"
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Logout")
                            font.pointSize: 7.5
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 9. Restart
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            control.close()
                            rootItem.showPowerDialog("restart")
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: control.inactiveCircleColor
                            border.width: !FishUI.Theme.darkMode ? 1 / FishUI.Dpi.ratio : 0
                            border.color: control.inactiveCircleBorderColor
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/"
                                    + control.inactiveIconDir + "system-reboot.svg"
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Restart")
                            font.pointSize: 7.5
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }

                // 10. Power Off
                Item {
                    Layout.fillWidth: true
                    height: 60

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            control.close()
                            rootItem.showPowerDialog("shutdown")
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter
                            width: 42
                            height: 42
                            radius: 21
                            color: "#FF3B30"
                            antialiasing: true

                            Image {
                                anchors.centerIn: parent
                                width: 22
                                height: 22
                                sourceSize: Qt.size(44, 44)
                                source: "qrc:/statusbar/images/dark/system-shutdown.svg"
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Power Off")
                            font.pointSize: 7.5
                            horizontalAlignment: Text.AlignHCenter
                            renderType: Text.NativeRendering
                            color: FishUI.Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}