import QtQuick 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "System Temp (°C)"
    property SystemInfo systemInfo: null

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Text {
            text: systemInfo ? (systemInfo.temperature.toFixed(1) + "°C") : "0.0°C"
            color: "#ffffff"
            font.pointSize: 20
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: {
                if (!systemInfo) return "Low"
                var temp = systemInfo.temperature
                return temp > 80 ? "High" : temp > 60 ? "Normal" : "Low"
            }
            color: {
                if (!systemInfo) return "#44ff44"
                var temp = systemInfo.temperature
                return temp > 80 ? "#ff4444" : temp > 60 ? "#ffaa00" : "#44ff44"
            }
            font.pointSize: 12
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
