import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: serverLogViewer
    color: "white"
    radius: 8
    border.color: "#bdc3c7"
    border.width: 1
    
    property var logs: []
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10
        
        Text {
            text: "📋 Activity Logs"
            font.pixelSize: 18
            font.bold: true
            color: "#2c3e50"
        }
        
        // Log display
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            TextArea {
                id: logArea
                width: parent.width
                height: parent.height
                text: logs.join('\n')
                readOnly: true
                font.family: "Monospace"
                font.pixelSize: 12
                background: Rectangle {
                    color: "#ecf0f1"
                    radius: 4
                }
                
                // Auto-scroll to bottom when logs change
                onTextChanged: {
                    if (text.length > 0) {
                        cursorPosition = text.length
                    }
                }
            }
        }
        
        // Log controls
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                text: "🗑️ Clear Logs"
                background: Rectangle {
                    color: "#f39c12"
                    radius: 4
                }
                onClicked: {
                    if (typeof clearLogs === 'function') {
                        clearLogs()
                    }
                }
            }
            
            Item { Layout.fillWidth: true }
            
            CheckBox {
                text: "Auto-scroll"
                checked: true
            }
        }
    }
} 