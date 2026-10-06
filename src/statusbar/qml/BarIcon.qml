import QtQuick 2.12
import QtQuick.Window
import QtQuick.Effects

// Bar icons sit straight on the wallpaper; same soft shadow the text gets.
Image {
    id: control

    property bool dropShadow: false

    layer.enabled: control.dropShadow
    layer.smooth: true
    layer.textureSize: Qt.size(Math.ceil(width * Window.devicePixelRatio), Math.ceil(height * Window.devicePixelRatio))
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 1
        blurMax: 6
        shadowBlur: 0.5
        shadowColor: "black"
        shadowOpacity: 0.25
    }
}
