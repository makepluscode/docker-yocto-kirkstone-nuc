import QtQuick 2.15
import QtQuick.Controls 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "Storage"
    property SystemInfo systemInfo: null

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        Text {
            text: systemInfo ? (systemInfo.rootPartitionUsagePercent.toFixed(1) + "%") : "0.0%"
            color: "#ffffff"
            font.pointSize: 20
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        ProgressBar {
            width: parent.width
            from: 0
            to: 100
            value: systemInfo ? systemInfo.rootPartitionUsagePercent : 0

            background: Rectangle {
                color: "#333333"
                radius: 3
            }

            contentItem: Rectangle {
                color: {
                    if (!systemInfo) return "#44ff44"
                    var usage = systemInfo.rootPartitionUsagePercent
                    return usage > 80 ? "#ff4444" : usage > 60 ? "#ffaa00" : "#44ff44"
                }
                radius: 3
            }
        }

        Text {
            text: systemInfo ? (systemInfo.formatBytes(systemInfo.rootPartitionUsed) + " / " + systemInfo.formatBytes(systemInfo.rootPartitionTotal)) : "0 B / 0 B"
            color: "#cccccc"
            font.pointSize: 9
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
