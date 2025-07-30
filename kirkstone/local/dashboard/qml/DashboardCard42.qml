import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Grub 1.0

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "#1a1a1a"
    border.color: "#444444"
    border.width: 2
    radius: 8
    
    property GrubManager grubManager: null
    
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
                text: "GRUB Info"
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
                    text: "Boot Configuration"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                }
                Row {
                    spacing: 10
                    Text { text: "ORDER:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: grubManager ? ("ORDER=" + grubManager.bootOrder) : "Loading..."; color: "#ffff88"; font.pointSize: 8; font.bold: true }
                }
                
                // Boot Order Section
                Text {
                    text: "Boot Order"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                    anchors.topMargin: 10
                }
                Row {
                    spacing: 10
                    Text { text: "ORDER:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text {
                        text: grubManager && grubManager.bootOrder ? (grubManager.bootOrder.replace(/ /g, " → ")) : "Loading..."
                        color: "#ffffff"
                        font.pointSize: 8
                        font.bold: true
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Default:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: grubManager ? grubManager.defaultEntry : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                Row {
                    spacing: 10
                    Text { text: "Timeout:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: grubManager ? grubManager.timeout : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
                
                // Slot Order Section
                Text {
                    text: "Slot Priority"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                    anchors.topMargin: 10
                }
                
                Row {
                    spacing: 10
                    Text { text: "Slot A:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: grubManager ? grubManager.slotAOrder : "Loading..."; 
                        color: grubManager ? (grubManager.slotAOrder === "A" ? "#44ff44" : "#ffffff") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                Row {
                    spacing: 10
                    Text { text: "Slot B:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { 
                        text: grubManager ? grubManager.slotBOrder : "Loading..."; 
                        color: grubManager ? (grubManager.slotBOrder === "B" ? "#44ff44" : "#ffffff") : "#ffffff"; 
                        font.pointSize: 8; 
                        font.bold: true 
                    }
                }
                
                // Version Section
                Text {
                    text: "GRUB Version"
                    color: "#44ff44"
                    font.pointSize: 10
                    font.bold: true
                    anchors.topMargin: 10
                }
                
                Row {
                    spacing: 10
                    Text { text: "Version:"; color: "#cccccc"; font.pointSize: 8; width: 80; horizontalAlignment: Text.AlignRight }
                    Text { text: grubManager ? grubManager.grubVersion : "Loading..."; color: "#ffffff"; font.pointSize: 8; }
                }
            }
        }
    }
} 