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
                text: "Version Info"
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
            spacing: 8
            
            Row {
                spacing: 10
                Text { text: "Hostname:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.hostname : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Kernel:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.kernelVersion : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Architecture:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.architecture : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Uptime:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.uptime : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Build Time:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.buildTime : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Yocto Ver:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: systemInfo ? systemInfo.yoctoVersion : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
        }
    }
} 