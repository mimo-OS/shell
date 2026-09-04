import QtQuick 2.12
import QtQuick.Window 2.12
import Qt5Compat.GraphicalEffects

import Cutefish.Appearance 1.0 as Appearance
import FishUI 1.0 as FishUI

// The desktop background: an image or a plain colour, whichever
// cutefish-services currently publishes.
Item {
    id: control

    Appearance.Wallpaper {
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
        // Empty until cutefish-services answers; "file://" alone is a bad URL.
        source: settings.path ? "file://" + settings.path : ""
        sourceSize: Qt.size(width * FishUI.Dpi.ratio,
                            height * FishUI.Dpi.ratio)
        fillMode: Image.PreserveAspectCrop
        clip: true
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
