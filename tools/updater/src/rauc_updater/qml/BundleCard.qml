import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Card {
    id: root
    title: qsTr("RAUC Bundle")
    icon: "📦"
    
    property var controller
    
    content: ColumnLayout {
        spacing: 15
        
        // Bundle file selection
        RowLayout {
            Layout.fillWidth: true
            
            TextField {
                id: bundlePathField
                text: controller ? controller.bundlePath : ""
                placeholderText: qsTr("Select RAUC bundle file (.raucb)")
                readOnly: true
                Layout.fillWidth: true
                
                background: Rectangle {
                    color: window.cardColor
                    border.color: window.darkTheme ? "#555555" : "#cccccc"
                    border.width: 1
                    radius: 4
                }
                color: window.textColor
            }
            
            Button {
                text: qsTr("Browse...")
                enabled: !controller.isUpdating
                onClicked: {
                    var path = controller.selectBundleFile()
                    if (path) {
                        bundlePathField.text = path
                    }
                }
                
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
        }
        
        // Bundle info (placeholder for future enhancement)
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: window.darkTheme ? "#444444" : "#f8f8f8"
            border.color: window.darkTheme ? "#666666" : "#dddddd"
            border.width: 1
            radius: 4
            visible: bundlePathField.text.length > 0
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Label {
                    text: qsTr("Bundle Information")
                    font.bold: true
                    color: window.textColor
                }
                
                Label {
                    text: bundlePathField.text ? qsTr("File: %1").arg(bundlePathField.text.split('/').pop()) : ""
                    color: window.textColor
                    font.pixelSize: 12
                }
                
                Label {
                    text: qsTr("Ready for installation")
                    color: window.successColor
                    font.pixelSize: 12
                }
            }
        }
        
        // Update button
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                id: updateButton
                text: controller.isUpdating ? qsTr("Cancel Update") : qsTr("Start Update")
                enabled: controller.isConnected && bundlePathField.text.length > 0
                Layout.preferredWidth: 150
                
                onClicked: {
                    if (controller.isUpdating) {
                        controller.cancelUpdate()
                    } else {
                        controller.startUpdate()
                    }
                }
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#cccccc"
                        var baseColor = controller.isUpdating ? window.warningColor : window.successColor
                        if (parent.pressed) return Qt.darker(baseColor, 1.2)
                        if (parent.hovered) return Qt.lighter(baseColor, 1.1)
                        return baseColor
                    }
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
            
            // Status indicator
            Label {
                text: {
                    if (!controller.isConnected) return qsTr("Connect first")
                    if (!bundlePathField.text) return qsTr("Select bundle file")
                    if (controller.isUpdating) return qsTr("Update in progress...")
                    return qsTr("Ready to update")
                }
                color: {
                    if (!controller.isConnected || !bundlePathField.text) return window.errorColor
                    if (controller.isUpdating) return window.warningColor
                    return window.successColor
                }
                font.pixelSize: 12
            }
        }
    }
}