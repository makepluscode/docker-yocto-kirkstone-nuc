import QtQuick 2.15
import QtQuick.Controls 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "CPU Load"
    property SystemInfo systemInfo: null
    
    // Timer as a property, not in content
    property Timer refreshTimer: Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: if (systemInfo) systemInfo.refresh()
    }
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        
        Text {
            text: systemInfo ? (systemInfo.cpuUsage.toFixed(1) + "%") : "0.0%"
            color: {
                if (!systemInfo) return "#ffffff"
                var usage = systemInfo.cpuUsage
                if (usage >= 80) return "#ff4444"      // Red for 80%+
                else if (usage >= 50) return "#4444ff"  // Blue for 50%+
                else if (usage >= 20) return "#44ff44"  // Green for 20%+
                else return "#ffff44"                   // Yellow for 0%+
            }
            font.pointSize: 18
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        ProgressBar {
            width: parent.width
            from: 0
            to: 100
            value: systemInfo ? systemInfo.cpuUsage : 0
            
            background: Rectangle {
                color: "#333333"
                radius: 3
            }
            
            contentItem: Rectangle {
                color: {
                    if (!systemInfo) return "#44ff44"
                    var usage = systemInfo.cpuUsage
                    return usage > 80 ? "#ff4444" : usage > 60 ? "#ffaa00" : "#44ff44"
                }
                radius: 3
            }
        }
        
        // CPU cores usage - compact layout for 6-column grid
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            
            // First row - cores 1-4
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                
                Repeater {
                    model: Math.min(4, systemInfo ? systemInfo.cpuCoreUsage.length : 0)
                    
                    Column {
                        spacing: 2
                        
                        Text {
                            text: "C" + (index + 1)
                            color: "#cccccc"
                            font.pointSize: 7
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: systemInfo ? systemInfo.cpuCoreUsage[index] + "%" : "0%"
                            color: "#ffffff"
                            font.pointSize: 8
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
            
            // Second row - cores 5-8 (if available)
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                visible: systemInfo && systemInfo.cpuCoreUsage.length > 4
                
                Repeater {
                    model: Math.min(4, Math.max(0, (systemInfo ? systemInfo.cpuCoreUsage.length : 0) - 4))
                    
                    Column {
                        spacing: 2
                        
                        Text {
                            text: "C" + (index + 5)
                            color: "#cccccc"
                            font.pointSize: 7
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        
                        Text {
                            text: systemInfo ? systemInfo.cpuCoreUsage[index + 4] + "%" : "0%"
                            color: "#ffffff"
                            font.pointSize: 8
                            font.bold: true
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }
    }
}