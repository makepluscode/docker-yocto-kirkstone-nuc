import QtQuick 2.15
import QtQuick.Layouts 1.15
import Rauc 1.0
import SystemInfo 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "#1a1a1a"
    border.color: "#444444"
    border.width: 2
    radius: 8
    
    property RaucManager raucManager: null
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
                anchors.centerIn: parent
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
                Text { text: "Current:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? ("Slot " + systemInfo.getCurrentBootSlot()) : "Loading..."; 
                    color: "#44ff44"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            Row {
                spacing: 10
                Text { text: "Order:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? systemInfo.getBootOrder() : "Loading..."; 
                    color: "#ffff44"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
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
            Row {
                spacing: 10
                Text { text: "A Status:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? systemInfo.getSlotAStatus() : "Loading..."; 
                    color: systemInfo ? (systemInfo.getSlotAStatus() === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            Row {
                spacing: 10
                Text { text: "B Status:"; color: "#cccccc"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? systemInfo.getSlotBStatus() : "Loading..."; 
                    color: systemInfo ? (systemInfo.getSlotBStatus() === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            // SW Update Status Section
            Rectangle {
                width: parent.width
                height: 1
                color: "#444444"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Row {
                spacing: 10
                Text { text: "SW Update:"; color: "#00ffff"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight; font.bold: true }
                Text { 
                    text: "Ready"; 
                    color: "#00ff00"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            Row {
                spacing: 10
                Text { text: "A Health:"; color: "#00ffff"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? (systemInfo.isSlotAHealthy() ? "Good" : "Bad") : "Unknown"; 
                    color: systemInfo ? (systemInfo.isSlotAHealthy() ? "#00ff00" : "#ff0000") : "#ffff00"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
            Row {
                spacing: 10
                Text { text: "B Health:"; color: "#00ffff"; font.pointSize: 9; width: 80; horizontalAlignment: Text.AlignRight }
                Text { 
                    text: systemInfo ? (systemInfo.isSlotBHealthy() ? "Good" : "Bad") : "Unknown"; 
                    color: systemInfo ? (systemInfo.isSlotBHealthy() ? "#00ff00" : "#ff0000") : "#ffff00"; 
                    font.pointSize: 9; 
                    font.bold: true 
                }
            }
        }
    }
} 