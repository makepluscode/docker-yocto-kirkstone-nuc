import QtQuick 2.15
import QtQuick.Layouts 1.15
import Rauc 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
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
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 15
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            
            Row {
                spacing: 10
                Text { text: "Compatible:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { text: raucManager ? raucManager.compatible : "Loading..."; color: "#ffffff"; font.pointSize: 9; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Booted:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { text: raucManager ? raucManager.booted : "Loading..."; color: "#ffffff"; font.pointSize: 9; font.bold: true }
            }
            Row {
                spacing: 10
                Text { text: "Slot A:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: raucManager ? (raucManager.slotAState + " (" + raucManager.slotAStatus + ")") : "Loading..."; 
                    color: raucManager ? (raucManager.slotAStatus === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            Row {
                spacing: 10
                Text { text: "Slot B:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: raucManager ? (raucManager.slotBState + " (" + raucManager.slotBStatus + ")") : "Loading..."; 
                    color: raucManager ? (raucManager.slotBStatus === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
        }
    }
} 