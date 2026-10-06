/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * Modern macOS Now Playing Media Card (Pre-Liquid Glass / Big Sur - Sonoma)
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Qt5Compat.GraphicalEffects
import FishUI 1.0 as FishUI
import Cutefish.Media 1.0

Item {
    id: control

    property bool available: mprisManager.availableServices.length > 0
    property bool isPlaying: currentService && mprisManager.playbackStatus === Mpris.Playing
    property alias currentService: mprisManager.currentService
    property var artUrlTag: Mpris.metadataToString(Mpris.ArtUrl)
    property var titleTag: Mpris.metadataToString(Mpris.Title)
    property var artistTag: Mpris.metadataToString(Mpris.Artist)

    property var cardBgColor: FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.08)
                                                    : Qt.rgba(1, 1, 1, 0.75)
    property var cardBorderColor: FishUI.Theme.darkMode ? Qt.rgba(255, 255, 255, 0.10)
                                                        : Qt.rgba(0, 0, 0, 0.05)

    MprisManager {
        id: mprisManager

        onCurrentServiceChanged: {
            control.updateInfo()
        }

        onMetadataChanged: {
            control.updateInfo()
        }
    }

    Component.onCompleted: { control.updateInfo() }

    function updateInfo() {
        var metadata = mprisManager.metadata || {}
        var titleAvailable = (titleTag in metadata) ? metadata[titleTag].toString() !== "" : false
        var artistAvailable = (artistTag in metadata) ? metadata[artistTag].toString() !== "" : false
        var artUrl = (artUrlTag in metadata) ? normalizeArtUrl(metadata[artUrlTag]) : ""

        control.visible = titleAvailable || artistAvailable || artUrl !== ""
        _songLabel.text = titleAvailable ? metadata[titleTag].toString() : ""
        _artistLabel.text = artistAvailable ? metadata[artistTag].toString() : ""
        artImage.source = artUrl
    }

    function normalizeArtUrl(value) {
        var url = value === undefined || value === null ? "" : String(value).trim()
        if (url.charAt(0) === "/")
            return "file://" + url
        return url
    }

    // Frosted Card Surface
    Rectangle {
        anchors.fill: parent
        color: control.cardBgColor
        radius: 14
        border.width: 1 / FishUI.Dpi.ratio
        border.color: control.cardBorderColor
        antialiasing: true
    }

    RowLayout {
        id: _mainLayout
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Album Art
        Item {
            Layout.preferredWidth: 52
            Layout.preferredHeight: 52
            Layout.alignment: Qt.AlignVCenter

            Image {
                id: defaultImage
                anchors.fill: parent
                source: "qrc:/statusbar/images/media-cover.svg"
                sourceSize: Qt.size(Math.ceil(52 * FishUI.Dpi.ratio), Math.ceil(52 * FishUI.Dpi.ratio))
                visible: !artImage.visible
                fillMode: Image.PreserveAspectCrop

                layer.enabled: true
                layer.smooth: true
                layer.textureSize: Qt.size(Math.ceil(width * FishUI.Dpi.ratio), Math.ceil(height * FishUI.Dpi.ratio))
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: 52
                        height: 52
                        radius: 10
                    }
                }
            }

            Image {
                id: artImage
                anchors.fill: parent
                visible: status === Image.Ready
                fillMode: Image.PreserveAspectCrop
                sourceSize: Qt.size(Math.ceil(256 * FishUI.Dpi.ratio), Math.ceil(256 * FishUI.Dpi.ratio))
                asynchronous: true

                layer.enabled: true
                layer.smooth: true
                layer.textureSize: Qt.size(Math.ceil(width * FishUI.Dpi.ratio), Math.ceil(height * FishUI.Dpi.ratio))
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: 52
                        height: 52
                        radius: 10
                    }
                }
            }

            // Subtle inner border over album art
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: "transparent"
                border.width: 1 / FishUI.Dpi.ratio
                border.pixelAligned: FishUI.Dpi.ratio > 1 ? false : true
                border.color: Qt.rgba(0, 0, 0, 0.15)
                antialiasing: true
            }
        }

        // Track & Artist Information
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Label {
                id: _songLabel
                Layout.fillWidth: true
                visible: _songLabel.text !== ""
                elide: Text.ElideRight
                font.bold: true
                font.pointSize: FishUI.Theme.fontSize
                color: FishUI.Theme.textColor
            }

            Label {
                id: _artistLabel
                Layout.fillWidth: true
                visible: _artistLabel.text !== ""
                elide: Text.ElideRight
                font.pointSize: FishUI.Theme.smallFont.pointSize
                color: FishUI.Theme.disabledTextColor
            }
        }

        // Playback Control Buttons
        RowLayout {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                width: 28
                height: 28
                radius: 14
                color: prevMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.20) : Qt.rgba(0, 0, 0, 0.15))
                                         : (prevMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.06)) : "transparent")
                visible: mprisManager.canGoPrevious
                antialiasing: true

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    sourceSize: Qt.size(Math.ceil(16 * Window.devicePixelRatio), Math.ceil(16 * Window.devicePixelRatio))
                    mipmap: false
                    smooth: true
                    antialiasing: true
                    source: "qrc:/statusbar/images/" + (FishUI.Theme.darkMode ? "dark" : "light") + "/media-skip-backward-symbolic.svg"
                }

                MouseArea {
                    id: prevMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: if (mprisManager.canGoPrevious) mprisManager.previous()
                }
            }

            Rectangle {
                width: 32
                height: 32
                radius: 16
                color: playMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.18))
                                         : (playMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.08))
                                                                    : (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.04)))
                visible: mprisManager.canPause || mprisManager.canPlay
                antialiasing: true

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    sourceSize: Qt.size(Math.ceil(16 * Window.devicePixelRatio), Math.ceil(16 * Window.devicePixelRatio))
                    mipmap: false
                    smooth: true
                    antialiasing: true
                    source: control.isPlaying ? "qrc:/statusbar/images/" + (FishUI.Theme.darkMode ? "dark" : "light") + "/media-playback-pause-symbolic.svg"
                                              : "qrc:/statusbar/images/" + (FishUI.Theme.darkMode ? "dark" : "light") + "/media-playback-start-symbolic.svg"
                }

                MouseArea {
                    id: playMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if ((control.isPlaying && mprisManager.canPause) || (!control.isPlaying && mprisManager.canPlay)) {
                            mprisManager.playPause()
                        }
                    }
                }
            }

            Rectangle {
                width: 28
                height: 28
                radius: 14
                color: nextMouse.pressed ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.20) : Qt.rgba(0, 0, 0, 0.15))
                                         : (nextMouse.containsMouse ? (FishUI.Theme.darkMode ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.06)) : "transparent")
                visible: mprisManager.canGoNext
                antialiasing: true

                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    sourceSize: Qt.size(Math.ceil(16 * Window.devicePixelRatio), Math.ceil(16 * Window.devicePixelRatio))
                    mipmap: false
                    smooth: true
                    antialiasing: true
                    source: "qrc:/statusbar/images/" + (FishUI.Theme.darkMode ? "dark" : "light") + "/media-skip-forward-symbolic.svg"
                }

                MouseArea {
                    id: nextMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: if (mprisManager.canGoNext) mprisManager.next()
                }
            }
        }
    }
}
