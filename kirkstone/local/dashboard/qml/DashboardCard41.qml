import QtQuick 2.15
import QtQuick.Controls 2.15
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
                text: "RAUC Status"
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
        
        // Content
        ScrollView {
            anchors.fill: parent
            anchors.margins: 15
            anchors.topMargin: 50
            
            Column {
                spacing: 8
                width: parent.width
                
                // Basic Info Section
                Text {
                    text: "Basic Information"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                }
                
                Row {
                    spacing: 10
                    Text { text: "Compatible:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: raucManager ? raucManager.compatible : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                Row {
                    spacing: 10
                    Text { text: "Variant:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: raucManager ? raucManager.variant : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                Row {
                    spacing: 10
                    Text { text: "Booted:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: raucManager ? raucManager.booted : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                
                // Slot A Section
                Text {
                    text: "Slot A (rootfs.0)"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                    anchors.topMargin: 10
                }
                
                Row {
                    spacing: 10
                    Text { text: "State:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: raucManager ? raucManager.slotAState : "Loading..."; 
                        color: raucManager ? (raucManager.slotAState === "booted" ? "#44ff44" : "#ffffff") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Status:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: raucManager ? raucManager.slotAStatus : "Loading..."; 
                        color: raucManager ? (raucManager.slotAStatus === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Device:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: raucManager ? raucManager.slotADevice : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                
                // Slot B Section
                Text {
                    text: "Slot B (rootfs.1)"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                    anchors.topMargin: 10
                }
                
                Row {
                    spacing: 10
                    Text { text: "State:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: raucManager ? raucManager.slotBState : "Loading..."; 
                        color: raucManager ? (raucManager.slotBState === "booted" ? "#44ff44" : "#ffffff") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Status:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: raucManager ? raucManager.slotBStatus : "Loading..."; 
                        color: raucManager ? (raucManager.slotBStatus === "good" ? "#44ff44" : "#ff4444") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Device:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: raucManager ? raucManager.slotBDevice : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
            }
        }
    }
} 