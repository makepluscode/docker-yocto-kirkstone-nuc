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
        spacing: 8
        
        CardInfoRow {
            label: "SW Version"
            value: systemInfo ? systemInfo.softwareVersion : ""
            labelWidth: 90
        }
    }
}