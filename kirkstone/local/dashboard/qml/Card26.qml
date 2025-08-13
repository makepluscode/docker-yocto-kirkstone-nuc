import QtQuick 2.15
import RaucSystem 1.0

DashboardCardBase {
    title: "RAUC Hawkbit Updater"
    property RaucSystemManager raucSystemManager: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        
        // Service status indicator
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#ffff44" // Yellow for unknown status
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "Hawkbit Client"
                color: "#ffffff"
                font.pointSize: 10
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // Status information
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 6
            
            CardInfoRow {
                label: "Service"
                value: "rauc-hawkbit-cpp.service"
                labelWidth: 60
                valueColor: "#cccccc"
            }
            
            CardInfoRow {
                label: "Status"
                value: "Monitoring for updates"
                labelWidth: 60
                valueColor: "#44ff44"
            }
            
            CardInfoRow {
                label: "Server"
                value: "hawkbit.example.com"
                labelWidth: 60
                valueColor: "#ffff44"
            }
            
            CardInfoRow {
                label: "Controller"
                value: "nuc-device-01"
                labelWidth: 60
                valueColor: "#888888"
            }
        }
        
        // Info message
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Automatic OTA updates\nvia Hawkbit server"
            color: "#888888"
            font.pointSize: 9
            horizontalAlignment: Text.AlignHCenter
        }
    }
}