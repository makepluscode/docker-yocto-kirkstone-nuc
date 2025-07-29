import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import SystemInfo 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "#1a1a1a"
    border.color: "#444444"
    border.width: 2
    radius: 8
    
    property SystemInfo systemInfo: null
    
    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: if (systemInfo) systemInfo.refresh()
    }
    
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
                anchors.centerIn: parent
                text: "Memory"
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
        
        // Content
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            
            Text {
                text: systemInfo ? (systemInfo.memoryUsage.toFixed(1) + "%") : "0.0%"
                color: "#ffffff"
                font.pointSize: 20
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            ProgressBar {
                width: parent.width
                from: 0
                to: 100
                value: systemInfo ? systemInfo.memoryUsage : 0
                
                background: Rectangle {
                    color: "#333333"
                    radius: 3
                }
                
                contentItem: Rectangle {
                    color: {
                        if (!systemInfo) return "#44ff44"
                        var usage = systemInfo.memoryUsage
                        return usage > 80 ? "#ff4444" : usage > 60 ? "#ffaa00" : "#44ff44"
                    }
                    radius: 3
                }
            }
            
            Text {
                text: systemInfo ? (systemInfo.formatBytes(systemInfo.usedMemory) + " / " + systemInfo.formatBytes(systemInfo.totalMemory)) : "0 B / 0 B"
                color: "#cccccc"
                font.pointSize: 9
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
} 