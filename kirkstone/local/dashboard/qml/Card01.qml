import QtQuick 2.15
import SystemInfo 1.0

DashboardCardBase {
    title: "System Info"
    property SystemInfo systemInfo: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        
        CardInfoRow {
            label: "Hostname"
            value: systemInfo ? systemInfo.hostname : ""
            labelWidth: 90
        }
        
        CardInfoRow {
            label: "Kernel"
            value: systemInfo ? systemInfo.kernelVersion : ""
            labelWidth: 90
        }
        
        CardInfoRow {
            label: "Architecture"
            value: systemInfo ? systemInfo.architecture : ""
            labelWidth: 90
        }
        
        CardInfoRow {
            label: "Build Time"
            value: systemInfo ? systemInfo.buildTime : ""
            labelWidth: 90
        }
        
        CardInfoRow {
            label: "Yocto Ver"
            value: systemInfo ? systemInfo.yoctoVersion : ""
            labelWidth: 90
        }
    }
}