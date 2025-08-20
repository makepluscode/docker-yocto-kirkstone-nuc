import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: updatePopup

    // Popup properties
    property bool isVisible: false
    property string status: "Initializing..."
    property double progress: 0.0
    property bool showProgress: true

    // Animation properties
    property int animationDuration: 300

    // Popup styling (1.2x size)
    width: 600
    height: 240
    color: "#2a2a2a"
    border.color: "#4a9eff"
    border.width: 4
    radius: 10

    // Center the popup
    anchors.centerIn: parent

    // Show/hide animation
    visible: opacity > 0
    opacity: isVisible ? 1.0 : 0.0
    scale: isVisible ? 1.0 : 0.8

    Behavior on opacity {
        NumberAnimation { duration: animationDuration }
    }

    Behavior on scale {
        NumberAnimation { duration: animationDuration }
    }

    // Background shadow effect
    Rectangle {
        anchors.fill: parent
        anchors.margins: -5
        color: "#000000"
        opacity: 0.3
        radius: parent.radius + 5
        z: -1
    }

    // Content layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 15

        // Header
        Row {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 26
                height: 26
                color: "#4a9eff"
                radius: 12
                Text {
                    anchors.centerIn: parent
                    text: "⟳"
                    color: "white"
                    font.pixelSize: 24
		    font.bold: true
                    RotationAnimation on rotation {
                        running: updatePopup.showProgress && updatePopup.progress < 100
                        from: 0
                        to: 360
                        duration: 1000
                        loops: Animation.Infinite
                    }
                }
            }

            Text {
		text: "Software Upgrade"
                color: "#ffffff"
                font.pixelSize: 28
                font.bold: true
		verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Status text
        Text {
            Layout.fillWidth: true
            text: updatePopup.status
            color: "#cccccc"
            font.pixelSize: 20
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // Progress bar
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: "#1a1a1a"
            border.color: "#555555"
            border.width: 2
            radius: 8
            visible: true

            Rectangle {
                id: progressFill
                height: parent.height - 2
                width: Math.max(10, (parent.width - 2) * (updatePopup.progress / 100))
                color: "#4a9eff"
                radius: parent.radius - 1
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 1

                Behavior on width {
                    NumberAnimation { duration: 200 }
                }
            }

            Text {
                anchors.centerIn: parent
                text: Math.round(updatePopup.progress) + "%"
                color: "#ffffff"
                font.pixelSize: 16
                font.bold: true
            }
        }

    }

    // Auto-hide after completion (optional)
    Timer {
        id: autoHideTimer
        interval: 5000
        running: false
        onTriggered: {
            if (updatePopup.progress >= 100 && !updatePopup.status.includes("Rebooting")) {
                updatePopup.isVisible = false
            }
        }
    }

    // Watch for completion to start auto-hide timer
    onProgressChanged: {
        if (progress >= 100 && isVisible && !updatePopup.status.includes("Rebooting")) {
            autoHideTimer.start()
        }
    }
}
