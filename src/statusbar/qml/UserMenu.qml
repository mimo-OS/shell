/*
 * Copyright (C) 2021 - 2022 CutefishOS Team.
 *
 * Author:     Kate Leet <kate@cutefishos.com>
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
import Cutefish.StatusBar 1.0
import Cutefish.Shell 1.0
import FishUI 1.0 as FishUI

FishUI.DesktopMenu {
    id: control

    PowerActions {
        id: actions
    }

    ProcessProvider {
        id: procProvider
    }

    FishUI.MenuItem {
        text: qsTr("About This Mac")
        onTriggered: process.startDetached("cutefish-settings", ["-m", "about"])
    }

    FishUI.MenuSeparator {}

    FishUI.MenuItem {
        text: qsTr("System Settings...")
        onTriggered: process.startDetached("cutefish-settings", [])
    }

    FishUI.MenuItem {
        text: qsTr("App Store...")
        onTriggered: {
            process.startDetached("cutefish-settings", ["-m", "update"])
        }
    }

    FishUI.MenuSeparator {}

    FishUI.MenuItem {
        text: qsTr("Force Quit...")
        onTriggered: {
            process.startDetached("qps", [])
        }
    }

    FishUI.MenuSeparator {}

    FishUI.MenuItem {
        text: qsTr("Sleep")
        onTriggered: actions.suspend()
    }

    FishUI.MenuItem {
        text: qsTr("Restart...")
        onTriggered: actions.reboot()
    }

    FishUI.MenuItem {
        text: qsTr("Shut Down...")
        onTriggered: actions.shutdown()
    }

    FishUI.MenuSeparator {}

    FishUI.MenuItem {
        text: qsTr("Lock Screen")
        onTriggered: actions.lockScreen()
    }

    FishUI.MenuItem {
        text: qsTr("Log Out...")
        onTriggered: actions.logout()
    }
}
