import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Card {
    id: root
    title: qsTr("Update Progress")
    icon: "⚡"
    
    property var controller
    property int progress: 0
    property string statusText: ""
    
    // Connect to controller signals
    Connections {
        target: controller
        function onProgressChanged(value) {
            root.progress = value
        }
        function onStatusChanged(status) {
            root.statusText = status
        }
    }
    
    content: ColumnLayout {
        spacing: 15
        
        // Status text
        Label {
            text: statusText || qsTr("Initializing...")
            color: window.textColor
            font.pixelSize: 14
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
        
        // Progress bar
        ProgressBar {
            id: progressBar
            Layout.fillWidth: true
            from: 0
            to: 100
            value: progress
            
            background: Rectangle {
                color: window.darkTheme ? "#444444" : "#e0e0e0"
                radius: 4
                implicitWidth: 200
                implicitHeight: 8
            }
            
            contentItem: Item {
                implicitWidth: 200
                implicitHeight: 8
                
                Rectangle {
                    width: progressBar.visualPosition * parent.width
                    height: parent.height
                    radius: 4
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: window.accentColor }
                        GradientStop { position: 1.0; color: Qt.lighter(window.accentColor, 1.2) }
                    }
                    
                    // Animated shine effect
                    Rectangle {
                        anchors.fill: parent
                        radius: 4
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, 0.2) }
                            GradientStop { position: 1.0; color: "transparent" }
                        }
                        
                        PropertyAnimation {
                            target: parent
                            property: "x"
                            from: -parent.width
                            to: parent.parent.width
                            duration: 2000
                            loops: Animation.Infinite
                            running: controller.isUpdating
                        }
                    }
                }
            }
        }
        
        // Progress percentage
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: qsTr("Progress:")
                color: window.textColor
                font.pixelSize: 12
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Label {
                text: progress + "%"
                color: window.accentColor
                font.bold: true
                font.pixelSize: 14
            }
        }
        
        // Time estimate (placeholder)
        Label {
            text: {
                if (progress <= 0) return qsTr("Estimating time...")
                if (progress >= 100) return qsTr("Completed!")
                
                // Simple time estimation based on progress
                var remaining = Math.round((100 - progress) * 2) // rough estimate
                if (remaining < 60) {
                    return qsTr("About %1 seconds remaining").arg(remaining)
                } else {
                    var minutes = Math.round(remaining / 60)
                    return qsTr("About %1 minutes remaining").arg(minutes)
                }
            }
            color: window.textColor
            font.pixelSize: 11
            opacity: 0.8
        }
    }
}