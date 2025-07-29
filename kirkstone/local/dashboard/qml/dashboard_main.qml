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
            
            // Left side - Title
            Text {
                text: "NUC System Dashboard"
                color: "#ffffff"
                font.pointSize: 14
                font.bold: true
                Layout.fillWidth: true
            }
            
            // Right side - Network status and time
            RowLayout {
                spacing: 20
                
                // Network status
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
                
                // Current time
                Text {
                    text: systemInfo.currentTime
                    color: "#ffffff"
                    font.pointSize: 12
                    font.bold: true
                }
            }
        }
    }

    // Main content area
    ScrollView {
        id: contentArea
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        
        contentWidth: contentGrid.width
        contentHeight: contentGrid.height
        
        GridLayout {
            id: contentGrid
            columns: 2
            rowSpacing: 15
            columnSpacing: 15
            width: contentArea.width - 20
            
            // System Information Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 20
                    
                    Row {
                        spacing: 10
                        Text { text: "Hostname:"; color: "#cccccc" }
                        Text { text: systemInfo.hostname; color: "#ffffff" }
                    }
                    Row {
                        spacing: 10
                        Text { text: "Kernel:"; color: "#cccccc" }
                        Text { text: systemInfo.kernelVersion; color: "#ffffff" }
                    }
                    Row {
                        spacing: 10
                        Text { text: "Architecture:"; color: "#cccccc" }
                        Text { text: systemInfo.architecture; color: "#ffffff" }
                    }
                    Row {
                        spacing: 10
                        Text { text: "Uptime:"; color: "#cccccc" }
                        Text { text: systemInfo.uptime; color: "#ffffff" }
                    }
                    Row {
                        spacing: 10
                        Text { text: "Temperature:"; color: "#cccccc" }
                        Text { text: systemInfo.temperature.toFixed(1) + "°C"; color: "#ffffff" }
                    }
                }
            }
            
            // CPU Usage Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: systemInfo.cpuUsage.toFixed(1) + "%"
                        color: "#ffffff"
                        font.pointSize: 24
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
                    
                    Text {
                        text: "CPU Load Average"
                        color: "#cccccc"
                        font.pointSize: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            
            // Memory Usage Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: systemInfo.memoryUsage.toFixed(1) + "%"
                        color: "#ffffff"
                        font.pointSize: 24
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
                    
                    Grid {
                        columns: 2
                        spacing: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                        
                        Text {
                            text: "Used: " + systemInfo.formatBytes(systemInfo.usedMemory)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                        
                        Text {
                            text: "Free: " + systemInfo.formatBytes(systemInfo.freeMemory)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                        
                        Text {
                            text: "Total: " + systemInfo.formatBytes(systemInfo.totalMemory)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                    }
                }
            }
            
            // Disk Usage Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: systemInfo.rootPartitionUsagePercent.toFixed(1) + "%"
                        color: "#ffffff"
                        font.pointSize: 24
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
                    
                    Grid {
                        columns: 2
                        spacing: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                        
                        Text {
                            text: "Used: " + systemInfo.formatBytes(systemInfo.rootPartitionUsed)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                        
                        Text {
                            text: "Free: " + systemInfo.formatBytes(systemInfo.rootPartitionFree)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                        
                        Text {
                            text: "Total: " + systemInfo.formatBytes(systemInfo.rootPartitionTotal)
                            color: "#cccccc"
                            font.pointSize: 9
                        }
                    }
                }
            }

            // RAUC Status Card
            Rectangle {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                color: "#1a1a1a"
                border.color: "#333333"
                border.width: 1
                radius: 8

                Column {
                    spacing: 10
                    anchors.margins: 12
                    anchors.fill: parent

                    Text {
                        text: "RAUC Status"
                        font.pixelSize: 24
                        font.bold: true
                        color: "#ffffff"
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 6
                        columnSpacing: 20

                        Text { text: "Booted Slot:"; color: "#cccccc" }
                        Text { text: raucManager.bootSlot; color: "#ffffff" }

                        Text { text: "Activated Slot:"; color: "#cccccc" }
                        Text { text: raucManager.activatedSlot; color: "#ffffff" }
                    }

                    Rectangle { height: 1; width: parent.width; color: "#444" }

                    ScrollView {
                        height: 160
                        TextArea {
                            width: parent.width
                            text: raucManager.statusText
                            readOnly: true
                            wrapMode: TextArea.Wrap
                        }
                    }

                    Row {
                        spacing: 12
                        Button { text: "Boot Slot A"; onClicked: raucManager.bootSlotA() }
                        Button { text: "Boot Slot B"; onClicked: raucManager.bootSlotB() }
                        Button { text: "Refresh";   onClicked: raucManager.refresh() }
                    }
                }
            }
        }
    }
} 