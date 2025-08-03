import QtQuick 2.15
import RaucSystem 1.0

DashboardCardBase {
    title: "Update Bundle Info"
    property RaucSystemManager raucSystemManager: null
    
    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        
        // Bundle status indicator
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: raucSystemManager && raucSystemManager.bundleExists ? "#44ff44" : "#ff4444"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: raucSystemManager && raucSystemManager.bundleExists ? "Bundle Available" : "No Bundle"
                color: "#ffffff"
                font.pointSize: 10
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // Bundle information (only show if bundle exists)
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 6
            visible: raucSystemManager && raucSystemManager.bundleExists
            
            CardInfoRow {
                label: "File"
                value: {
                    if (!raucSystemManager || !raucSystemManager.bundleExists) return ""
                    var path = raucSystemManager.bundlePath
                    return path.split('/').pop() // Get filename only
                }
                labelWidth: 60
                valueColor: "#44ff44"
            }
            
            CardInfoRow {
                label: "Size"
                value: raucSystemManager ? raucSystemManager.bundleSizeFormatted : "0 B"
                labelWidth: 60
                valueColor: "#ffff44"
            }
            
            CardInfoRow {
                label: "Modified"
                value: raucSystemManager ? raucSystemManager.bundleModified : ""
                labelWidth: 60
                valueColor: "#cccccc"
            }
            
            CardInfoRow {
                label: "Path"
                value: raucSystemManager ? raucSystemManager.bundlePath : ""
                labelWidth: 60
                valueColor: "#888888"
            }
        }
        
        // No bundle message
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Place .raucb file in\n/data directory"
            color: "#888888"
            font.pointSize: 9
            horizontalAlignment: Text.AlignHCenter
            visible: !raucSystemManager || !raucSystemManager.bundleExists
        }
    }
}