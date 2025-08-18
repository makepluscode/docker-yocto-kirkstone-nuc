import QtQuick 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "SW Version"
    property SystemInfo systemInfo: null

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        Text {
            text: systemInfo ? systemInfo.softwareVersion : "Unknown"
            color: "#ffffff"
            font.pointSize: 16
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
