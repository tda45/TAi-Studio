import QtQuick
import QtQuick.Window

Window {
    id: splash

    width: 500
    height: 300

    visible: true
    color: "transparent"

    flags: Qt.FramelessWindowHint
         | Qt.WindowStaysOnTopHint

    modality: Qt.ApplicationModal

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: "#0b0b0b"

        AnimatedImage {
            id: gif
            anchors.centerIn: parent

            width: 220
            height: 220

            source: "qrc:/gpt4all/icons/loading.gif"
            playing: true
        }

        Text {
            text: "TAi Studio"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: gif.bottom
            anchors.topMargin: 10

            color: "#ff2020"
            font.pixelSize: 28
            font.bold: true
        }
    }
}