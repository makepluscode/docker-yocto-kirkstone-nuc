import QtQuick 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "System Uptime"
    property SystemInfo systemInfo: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        
        CardInfoRow {
            label: "Uptime"
            value: systemInfo ? systemInfo.uptime : "Loading..."
            valueColor: "#44ff44"
            labelWidth: 60
        }
    }
}