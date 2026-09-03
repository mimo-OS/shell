/*
 * A FishUI desktop menu that shows a QMenu exported over DBus - an
 * application menu or a system tray menu - through QMenuModel.
 *
 * Submenus are the same component again, loaded by URL so that the file may
 * refer to itself, and only once the menu they hang off is on screen.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12

import Cutefish.StatusBar 1.0
import FishUI 1.0 as FishUI

FishUI.DesktopMenu {
    id: control

    property var menuModel: null

    // A DBus menu fills itself in only after the exporter has been told that
    // it is about to be shown, and expects to hear about the close as well.
    // The menu bar swaps the model of a popup that stays open, so this
    // follows the model too, not only the visibility.
    property var shownModel: null

    function syncShownModel() {
        var shown = control.visible ? control.menuModel : null
        if (shown === control.shownModel)
            return

        if (control.shownModel)
            control.shownModel.aboutToHide()

        control.shownModel = shown

        if (shown)
            shown.aboutToShow()
    }

    onMenuModelChanged: control.syncShownModel()

    Connections {
        target: control

        function onVisibleChanged() {
            control.syncShownModel()
        }
    }

    Repeater {
        model: control.menuModel

        delegate: Loader {
            id: _row

            Layout.fillWidth: true
            sourceComponent: model.isSeparator ? _separatorComponent : _itemComponent

            Component {
                id: _separatorComponent

                FishUI.MenuSeparator {}
            }

            Component {
                id: _itemComponent

                FishUI.MenuItem {
                    // The label reads "&" as a mnemonic marker and would
                    // underline the next character, so an ampersand that is
                    // part of the text has to be doubled.
                    text: model.text.replace(/&/g, "&&")
                    icon.name: model.iconName
                    enabled: model.isEnabled
                    checkable: model.isCheckable
                    checked: model.isChecked
                    // The submenu popup does not exist until this menu is
                    // shown, so the arrow cannot be deduced from it.
                    hasChildMenu: model.hasSubmenu
                    childMenu: _submenuLoader.item

                    Loader {
                        id: _submenuLoader

                        // One window per submenu, built a level at a time as
                        // the menu it belongs to opens - and kept afterwards:
                        // tearing the popup down while the menu closes would
                        // cut short the activation of the row just clicked.
                        active: false
                        source: active ? Qt.resolvedUrl("QMenuPopup.qml") : ""

                        onLoaded: item.menuModel = control.menuModel.submenu(index)

                        function createWhenShown() {
                            if (control.visible && model.hasSubmenu)
                                _submenuLoader.active = true
                        }

                        // A row can be created while its menu is already open,
                        // which is what happens when the menu bar swaps the
                        // model of the popup it keeps.
                        Component.onCompleted: createWhenShown()

                        Connections {
                            target: control

                            function onVisibleChanged() {
                                _submenuLoader.createWhenShown()
                            }
                        }

                        // A menu that updates while it is open recreates its
                        // rows. Closing the submenu first keeps the
                        // compositor from being left with a popup whose
                        // parent surface has gone.
                        Component.onDestruction: {
                            if (item && item.visible)
                                item.dismissPopup()
                        }
                    }

                    onTriggered: {
                        if (model.hasSubmenu)
                            return

                        control.menuModel.trigger(index)
                        control.dismissAllPopups()
                    }
                }
            }
        }
    }
}
