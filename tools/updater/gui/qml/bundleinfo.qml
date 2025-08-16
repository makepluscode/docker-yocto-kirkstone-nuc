import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: bundleInfoPanel
    color: "white"
    radius: 8
    border.color: "#bdc3c7"
    border.width: 1
    
    property var latestBundle: null
    signal selectBundleClicked()
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10
        
        Text {
            text: "📦 Latest Bundle"
            font.pixelSize: 18
            font.bold: true
            color: "#2c3e50"
        }
        
        // Latest bundle display
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#e3f2fd"
            radius: 6
            border.color: "#2196f3"
            border.width: 2
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 8
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#2196f3"
                    }
                    
                    Text {
                        text: "⭐ Latest Bundle Selected"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2196f3"
                    }
                }
                
                Text {
                    text: "Name: " + (latestBundle ? latestBundle.name : "No bundle available")
                    font.pixelSize: 14
                    color: "#2c3e50"
                    elide: Text.ElideRight
                }
                
                Text {
                    text: "Version: " + (latestBundle ? latestBundle.version : "Unknown")
                    font.pixelSize: 14
                    color: "#2c3e50"
                }
                
                Text {
                    text: "Size: " + (latestBundle ? latestBundle.size_mb + " MB" : "0 MB") + " | Created: " + (latestBundle ? latestBundle.mtime_str : "Unknown")
                    font.pixelSize: 14
                    color: "#2c3e50"
                    elide: Text.ElideRight
                }
                
                Item { Layout.fillHeight: true }
                
                // Select Bundle button
                Button {
                    Layout.fillWidth: true
                    text: "📁 Select Bundle"
                    background: Rectangle {
                        color: "#3498db"
                        radius: 4
                    }
                    onClicked: {
                        selectBundleClicked()
                    }
                }
            }
        }
    }
} 