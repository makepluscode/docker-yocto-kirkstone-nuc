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
            
            // Version Info Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                        anchors.fill: parent
                        anchors.margins: 15
                        anchors.topMargin: 50
                        spacing: 8
                        
                        Row {
                            spacing: 10
                            Text { text: "Hostname:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: systemInfo.hostname; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                        Row {
                            spacing: 10
                            Text { text: "Kernel:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: systemInfo.kernelVersion; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                        Row {
                            spacing: 10
                            Text { text: "Architecture:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: systemInfo.architecture; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                        Row {
                            spacing: 10
                            Text { text: "Uptime:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: systemInfo.uptime; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                    }
                }
            }
            
            // CPU Load Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            text: systemInfo.cpuUsage.toFixed(1) + "%"
                            color: "#ffffff"
                            font.pointSize: 20
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        ProgressBar {
                            width: parent.width
                            from: 0
                            to: 100
                            value: systemInfo.cpuUsage
                            
                            background: Rectangle {
                                color: "#333333"
                                radius: 3
                            }
                            
                            contentItem: Rectangle {
                                color: systemInfo.cpuUsage > 80 ? "#ff4444" : 
                                       systemInfo.cpuUsage > 60 ? "#ffaa00" : "#44ff44"
                                radius: 3
                            }
                        }
                    }
                }
            }
            
            // Memory Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            text: "Memory"
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
                            text: systemInfo.memoryUsage.toFixed(1) + "%"
                            color: "#ffffff"
                            font.pointSize: 20
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        ProgressBar {
                            width: parent.width
                            from: 0
                            to: 100
                            value: systemInfo.memoryUsage
                            
                            background: Rectangle {
                                color: "#333333"
                                radius: 3
                            }
                            
                            contentItem: Rectangle {
                                color: systemInfo.memoryUsage > 80 ? "#ff4444" : 
                                       systemInfo.memoryUsage > 60 ? "#ffaa00" : "#44ff44"
                                radius: 3
                            }
                        }
                        
                        Text {
                            text: systemInfo.formatBytes(systemInfo.usedMemory) + " / " + systemInfo.formatBytes(systemInfo.totalMemory)
                            color: "#cccccc"
                            font.pointSize: 9
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
            
            // Storage Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            text: "Storage"
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
                            text: systemInfo.rootPartitionUsagePercent.toFixed(1) + "%"
                            color: "#ffffff"
                            font.pointSize: 20
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        ProgressBar {
                            width: parent.width
                            from: 0
                            to: 100
                            value: systemInfo.rootPartitionUsagePercent
                            
                            background: Rectangle {
                                color: "#333333"
                                radius: 3
                            }
                            
                            contentItem: Rectangle {
                                color: systemInfo.rootPartitionUsagePercent > 80 ? "#ff4444" : 
                                       systemInfo.rootPartitionUsagePercent > 60 ? "#ffaa00" : "#44ff44"
                                radius: 3
                            }
                        }
                        
                        Text {
                            text: systemInfo.formatBytes(systemInfo.rootPartitionUsed) + " / " + systemInfo.formatBytes(systemInfo.rootPartitionTotal)
                            color: "#cccccc"
                            font.pointSize: 9
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
            
            // System Temp Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                        anchors.fill: parent
                        anchors.margins: 15
                        anchors.topMargin: 50
                        spacing: 10
                        
                        Text {
                            text: systemInfo.temperature.toFixed(1) + "°C"
                            color: "#ffffff"
                            font.pointSize: 24
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: systemInfo.temperature > 80 ? "High" : 
                                  systemInfo.temperature > 60 ? "Normal" : "Low"
                            color: systemInfo.temperature > 80 ? "#ff4444" : 
                                   systemInfo.temperature > 60 ? "#ffaa00" : "#44ff44"
                            font.pointSize: 12
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
            
            // Empty Card 1
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            text: "Empty Card"
                            color: "#ffffff"
                            font.pointSize: 12
                            font.bold: true
                        }
                    }
                    
                    // Content
                    Text {
                        anchors.centerIn: parent
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 12
                    }
                }
            }
            
            // Booting Info Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            Text { text: raucManager.bootSlot; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                        Row {
                            spacing: 10
                            Text { text: "Activated Slot:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: raucManager.activatedSlot; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                        Row {
                            spacing: 10
                            Text { text: "Status:"; color: "#cccccc"; font.pointSize: 10; width: 100; horizontalAlignment: Text.AlignRight }
                            Text { text: raucManager.statusText.split('\n')[0] || "Unknown"; color: "#ffffff"; font.pointSize: 10; font.bold: true }
                        }
                    }
                }
            }
            
            // Empty Card 2
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
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
                            text: "Empty Card"
                            color: "#ffffff"
                            font.pointSize: 12
                            font.bold: true
                        }
                    }
                    
                    // Content
                    Text {
                        anchors.centerIn: parent
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 12
                    }
                }
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
                
                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }
                
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    
                    Text {
                        text: "F4"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                enabled: false
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
                        text: "F5"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                enabled: false
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
                        text: "F6"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                enabled: false
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
                        text: "F7"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                enabled: false
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
                        text: "F8"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: "Empty"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                enabled: false
            }
        }
    }
} 