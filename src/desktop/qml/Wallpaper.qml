import QtQuick 2.12
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects

import Cutefish.System 1.0 as System
import FishUI 1.0 as FishUI

// The desktop background: an image or a plain colour, whichever
// cutefish-settings currently publishes.
Item {
    id: control

    System.Wallpaper {
        id: settings
    }

    Rectangle {
        anchors.fill: parent
        color: settings.color
        visible: settings.backgroundVisible && settings.type !== 0
    }

    Image {
        id: image
        anchors.fill: parent
        source: "file://" + settings.path
        sourceSize: Qt.size(width * Screen.devicePixelRatio,
                            height * Screen.devicePixelRatio)
        fillMode: Image.PreserveAspectCrop
        clip: true
        cache: false
        visible: settings.backgroundVisible && settings.type === 0
    }

    ColorOverlay {
        anchors.fill: image
        source: image
        visible: image.visible
        color: "#000000"
        opacity: FishUI.Theme.darkMode && settings.dimsWallpaper ? 0.4 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }
    }
}
