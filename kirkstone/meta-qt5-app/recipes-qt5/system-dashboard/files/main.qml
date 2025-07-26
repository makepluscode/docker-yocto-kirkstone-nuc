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
    title: qsTr("System Dashboard")
    color: "#000000"

    SystemInfo {
        id: systemInfo
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
            DashboardCard {
                title: "System Information"
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                
                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 20
                    
                    InfoRow { label: "Hostname:"; value: systemInfo.hostname }
                    InfoRow { label: "Kernel:"; value: systemInfo.kernelVersion }
                    InfoRow { label: "Architecture:"; value: systemInfo.architecture }
                    InfoRow { label: "Uptime:"; value: systemInfo.uptime }
                    InfoRow { label: "Temperature:"; value: systemInfo.temperature.toFixed(1) + "°C" }
                }
            }
            
            // CPU Usage Card
            DashboardCard {
                title: "CPU Usage"
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                
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
            DashboardCard {
                title: "Memory Usage"
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                
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
            DashboardCard {
                title: "Root Filesystem"
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                
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
            RaucCard {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredHeight: 300
            }
        }
    }
} 