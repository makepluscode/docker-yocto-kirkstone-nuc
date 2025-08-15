import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Card {
    id: root
    title: qsTr("Connection Settings")
    icon: "🔗"
    
    property var controller
    
    content: ColumnLayout {
        spacing: 15
        
        // Connection settings
        GridLayout {
            columns: 2
            columnSpacing: 15
            rowSpacing: 10
            Layout.fillWidth: true
            
            Label {
                text: qsTr("Host IP:")
                color: window.textColor
            }
            
            TextField {
                id: hostField
                text: controller ? controller.host : "192.168.1.100"
                placeholderText: qsTr("e.g., 192.168.1.100")
                Layout.fillWidth: true
                onTextChanged: if (controller) controller.host = text
                
                background: Rectangle {
                    color: window.cardColor
                    border.color: parent.activeFocus ? window.accentColor : (window.darkTheme ? "#555555" : "#cccccc")
                    border.width: parent.activeFocus ? 2 : 1
                    radius: 4
                }
                color: window.textColor
            }
            
            Label {
                text: qsTr("Username:")
                color: window.textColor
            }
            
            TextField {
                id: usernameField
                text: controller ? controller.username : "root"
                placeholderText: qsTr("e.g., root")
                Layout.fillWidth: true
                onTextChanged: if (controller) controller.username = text
                
                background: Rectangle {
                    color: window.cardColor
                    border.color: parent.activeFocus ? window.accentColor : (window.darkTheme ? "#555555" : "#cccccc")
                    border.width: parent.activeFocus ? 2 : 1
                    radius: 4
                }
                color: window.textColor
            }
            
            Label {
                text: qsTr("Port:")
                color: window.textColor
            }
            
            SpinBox {
                id: portField
                from: 1
                to: 65535
                value: controller ? controller.port : 22
                onValueChanged: if (controller) controller.port = value
                
                background: Rectangle {
                    color: window.cardColor
                    border.color: parent.activeFocus ? window.accentColor : (window.darkTheme ? "#555555" : "#cccccc")
                    border.width: parent.activeFocus ? 2 : 1
                    radius: 4
                }
                
                contentItem: TextInput {
                    text: parent.textFromValue(parent.value, parent.locale)
                    font: parent.font
                    color: window.textColor
                    selectionColor: window.accentColor
                    selectedTextColor: "white"
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    readOnly: !parent.editable
                    validator: parent.validator
                    inputMethodHints: parent.inputMethodHints
                }
            }
        }
        
        // Connection button and status
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                id: testButton
                text: qsTr("Test Connection")
                enabled: !controller.isUpdating
                onClicked: controller.testConnection()
                
                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? Qt.darker(window.accentColor, 1.2) : (parent.hovered ? Qt.lighter(window.accentColor, 1.1) : window.accentColor)) : "#cccccc"
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
            
            Row {
                spacing: 5
                
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: controller.isConnected ? window.successColor : window.errorColor
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Label {
                    text: controller.isConnected ? qsTr("Connected") : qsTr("Not Connected")
                    color: controller.isConnected ? window.successColor : window.errorColor
                    font.bold: true
                }
            }
        }
    }
}