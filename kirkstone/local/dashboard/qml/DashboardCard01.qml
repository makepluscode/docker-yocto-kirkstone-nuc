import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import SystemInfo 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 150
    color: "#1a1a1a"
    border.color: "#444444"
    border.width: 2
    radius: 8
    
    property SystemInfo systemInfo: null
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // Title bar
        Rectangle {
            width: parent.width
            height: 35
            color: "#2a2a2a"
            radius: 8
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                text: "CPU Load"
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
        
        // Content
        Column {
            anchors.fill: parent
            anchors.margins: 15
            anchors.topMargin: 50
            spacing: 10
            
            Text {
                text: systemInfo ? (systemInfo.cpuUsage.toFixed(1) + "%") : "0.0%"
                color: "#ffffff"
                font.pointSize: 20
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            ProgressBar {
                width: parent.width
                from: 0
                to: 100
                value: systemInfo ? systemInfo.cpuUsage : 0
                
                background: Rectangle {
                    color: "#333333"
                    radius: 3
                }
                
                contentItem: Rectangle {
                    color: {
                        if (!systemInfo) return "#44ff44"
                        var usage = systemInfo.cpuUsage
                        return usage > 80 ? "#ff4444" : usage > 60 ? "#ffaa00" : "#44ff44"
                    }
                    radius: 3
                }
            }
        }
    }
} 