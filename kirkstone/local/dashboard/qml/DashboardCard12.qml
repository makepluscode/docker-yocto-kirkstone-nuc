import QtQuick 2.15
import QtQuick.Layouts 1.15
import Rauc 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 150
    color: "#1a1a1a"
    border.color: "#444444"
    border.width: 2
    radius: 8
    
    property RaucManager raucManager: null
    
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
                text: "Booting Info"
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
            spacing: 8
            
            Row {
                spacing: 10
                Text { text: "Booted Slot:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: raucManager ? raucManager.bootSlot : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Activated Slot:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: raucManager ? raucManager.activatedSlot : ""; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Status:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                Text { text: raucManager ? (raucManager.statusText.split('\n')[0] || "Unknown") : "Unknown"; color: "#ffffff"; font.pointSize: 10; font.bold: true }
            }
        }
    }
} 