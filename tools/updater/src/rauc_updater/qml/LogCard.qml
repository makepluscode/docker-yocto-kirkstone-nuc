import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Card {
    id: root
    title: qsTr("Activity Log")
    icon: "📋"
    
    property var controller
    
    function appendLog(message) {
        var timestamp = new Date().toLocaleTimeString()
        var logEntry = "[" + timestamp + "] " + message + "\n"
        logTextArea.append(logEntry)
    }
    
    function clearLog() {
        logTextArea.clear()
    }
    
    // Connect to controller log messages
    Connections {
        target: controller
        function onLogMessage(message) {
            var timestamp = new Date().toLocaleTimeString()
            var logEntry = "[" + timestamp + "] " + message + "\n"
            logTextArea.append(logEntry)
        }
    }
    
    content: ColumnLayout {
        spacing: 10
        
        // Log controls
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                text: qsTr("Clear Log")
                enabled: logTextArea.text.length > 0
                onClicked: logTextArea.clear()
                
                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? "#d32f2f" : (parent.hovered ? "#f44336" : "#ff5252")) : "#cccccc"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: parent.enabled ? "white" : "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Label {
                text: qsTr("Auto-scroll")
                color: window.textColor
                font.pixelSize: 12
            }
            
            Switch {
                id: autoScrollSwitch
                checked: true
                
                indicator: Rectangle {
                    implicitWidth: 40
                    implicitHeight: 20
                    x: parent.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 10
                    color: parent.checked ? window.accentColor : (window.darkTheme ? "#666666" : "#cccccc")
                    border.color: parent.checked ? window.accentColor : (window.darkTheme ? "#888888" : "#999999")
                    
                    Rectangle {
                        x: parent.parent.checked ? parent.width - width : 0
                        width: 18
                        height: 18
                        radius: 9
                        anchors.verticalCenter: parent.verticalCenter
                        color: "white"
                        border.color: parent.parent.checked ? window.accentColor : (window.darkTheme ? "#888888" : "#999999")
                        
                        Behavior on x {
                            PropertyAnimation { duration: 100 }
                        }
                    }
                }
            }
        }
        
        // Log text area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 200
            
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            
            TextArea {
                id: logTextArea
                readOnly: true
                wrapMode: TextArea.Wrap
                selectByMouse: true
                font.family: "Consolas, Monaco, monospace"
                font.pixelSize: 11
                color: window.textColor
                
                background: Rectangle {
                    color: window.darkTheme ? "#1e1e1e" : "#fafafa"
                    border.color: window.darkTheme ? "#555555" : "#dddddd"
                    border.width: 1
                    radius: 4
                }
                
                // Auto-scroll to bottom when new content is added
                onTextChanged: {
                    if (autoScrollSwitch.checked) {
                        Qt.callLater(function() {
                            logTextArea.cursorPosition = logTextArea.length
                        })
                    }
                }
                
                // Initial welcome message
                Component.onCompleted: {
                    var timestamp = new Date().toLocaleTimeString()
                    append("[" + timestamp + "] RAUC Updater started\n")
                    append("[" + timestamp + "] Ready to connect to target device\n")
                }
                
                // Context menu for copy/select all
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: contextMenu.popup()
                    
                    Menu {
                        id: contextMenu
                        
                        MenuItem {
                            text: qsTr("Select All")
                            enabled: logTextArea.text.length > 0
                            onTriggered: logTextArea.selectAll()
                        }
                        
                        MenuItem {
                            text: qsTr("Copy")
                            enabled: logTextArea.selectedText.length > 0
                            onTriggered: logTextArea.copy()
                        }
                        
                        MenuSeparator {}
                        
                        MenuItem {
                            text: qsTr("Clear All")
                            enabled: logTextArea.text.length > 0
                            onTriggered: logTextArea.clear()
                        }
                    }
                }
            }
        }
        
        // Log statistics
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: qsTr("Lines: %1").arg(logTextArea.text.split('\n').length - 1)
                color: window.textColor
                font.pixelSize: 10
                opacity: 0.7
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Label {
                text: qsTr("Characters: %1").arg(logTextArea.text.length)
                color: window.textColor
                font.pixelSize: 10
                opacity: 0.7
            }
        }
    }
}