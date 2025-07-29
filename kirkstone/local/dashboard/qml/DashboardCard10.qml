import QtQuick 2.15
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
                text: "System Temp (°C)"
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
                text: systemInfo ? (systemInfo.temperature.toFixed(1) + "°C") : "0.0°C"
                color: "#ffffff"
                font.pointSize: 24
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
            
            Text {
                text: {
                    if (!systemInfo) return "Low"
                    var temp = systemInfo.temperature
                    return temp > 80 ? "High" : temp > 60 ? "Normal" : "Low"
                }
                color: {
                    if (!systemInfo) return "#44ff44"
                    var temp = systemInfo.temperature
                    return temp > 80 ? "#ff4444" : temp > 60 ? "#ffaa00" : "#44ff44"
                }
                font.pointSize: 12
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
} 