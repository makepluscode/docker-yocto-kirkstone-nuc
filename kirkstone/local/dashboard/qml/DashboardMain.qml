import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import SystemInfo 1.0
import Rauc 1.0

ApplicationWindow {
    id: mainWindow
    width: 1024
    height: 768
    visible: true
    color: "#000000"

    SystemInfo {
        id: systemInfo
    }

    RaucManager {
        id: raucManager
    }

    // Status Bar at top
    Rectangle {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "#1a1a1a"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            // Left side - Network status
            Row {
                spacing: 5
                
                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: systemInfo.networkConnected ? "#00ff00" : "#ff0000"
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Text {
                    text: systemInfo.networkConnected ? 
                          systemInfo.networkInterface + " (" + systemInfo.ipAddress + ")" : 
                          "Disconnected"
                    color: "#ffffff"
                    font.pointSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            // Center - Title
            Text {
                text: "NUC System Dashboard"
                color: "#ffffff"
                font.pointSize: 14
                font.bold: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            
            // Right side - Current time
            Text {
                text: systemInfo.currentTime
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
    }

    // Main content area
    ScrollView {
        id: contentArea
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonArea.top
        anchors.margins: 10
        
        contentWidth: contentGrid.width
        contentHeight: contentGrid.height
        
        GridLayout {
            id: contentGrid
            columns: 4
            rowSpacing: 15
            columnSpacing: 15
            width: contentArea.width - 20
            
            // Row 1: Cards 00-03
            DashboardCard00 {
                systemInfo: systemInfo
            }
            
            DashboardCard01 {
                systemInfo: systemInfo
            }
            
            DashboardCard02 {
                systemInfo: systemInfo
            }
            
            DashboardCard03 {
                systemInfo: systemInfo
            }
            
            // Row 2: Cards 10-13
            DashboardCard10 {
                systemInfo: systemInfo
            }
            
            DashboardCard11 {
                // Empty card - no properties needed
            }
            
            DashboardCard12 {
                raucManager: raucManager
            }
            
            DashboardCard13 {
                // Empty card - no properties needed
            }
            
            // Row 3: Cards 20-23
            DashboardCard20 {
                // Empty card - no properties needed
            }
            
            DashboardCard21 {
                // Empty card - no properties needed
            }
            
            DashboardCard22 {
                // Empty card - no properties needed
            }
            
            DashboardCard23 {
                // Empty card - no properties needed
            }
        }
    }
    
    // Button area at bottom
    Rectangle {
        id: buttonArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#1a1a1a"
        border.color: "#333333"
        border.width: 1
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                
                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F1"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Refresh"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                onClicked: {
                    systemInfo.refresh()
                    raucManager.refresh()
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                
                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F2"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "A Boot"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                onClicked: raucManager.bootSlotA()
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                
                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F3"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "B Boot"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                onClicked: raucManager.bootSlotB()
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false
                
                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F4"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false
                
                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F5"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false
                
                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F6"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false
                
                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F7"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false
                
                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F8"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }
} 