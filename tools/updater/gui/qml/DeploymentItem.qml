import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: deploymentItem
    height: 80
    color: deployment.active ? "#e8f5e8" : "#f8f9fa"
    radius: 6
    border.color: deployment.active ? "#27ae60" : "#dee2e6"
    border.width: 1
    
    property var deployment: ({})
    signal toggleDeployment()
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Status indicator
        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: deployment.active ? "#27ae60" : "#6c757d"
        }
        
        // Deployment info
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            
            Text {
                text: deployment.name || "Unnamed Deployment"
                font.pixelSize: 14
                font.bold: true
                color: "#2c3e50"
                elide: Text.ElideRight
            }
            
            Text {
                text: "Version: " + (deployment.version || "Unknown")
                font.pixelSize: 12
                color: "#6c757d"
            }
            
            Text {
                text: "Bundle: " + (deployment.filename || "No file")
                font.pixelSize: 11
                color: "#6c757d"
                elide: Text.ElideRight
            }
        }
        
        // Toggle button
        Switch {
            checked: deployment.active || false
            onCheckedChanged: {
                if (checked !== deployment.active) {
                    toggleDeployment()
                }
            }
        }
    }
    
    // Hover effect
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            parent.scale = 1.02
        }
        onExited: {
            parent.scale = 1.0
        }
        
        Behavior on scale {
            NumberAnimation { duration: 150 }
        }
    }
} 