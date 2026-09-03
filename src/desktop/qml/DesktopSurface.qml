import QtQuick 2.12

// The desktop of one screen: the shell draws the background, and the file
// manager's DesktopView -- icons, selection, drag and drop, menus and every
// file operation -- goes on top of it, on the primary screen only.
Item {
    id: root

    Wallpaper {
        anchors.fill: parent
    }

    Loader {
        anchors.fill: parent
        active: surface.primary
        source: "DesktopIcons.qml"
    }
}
