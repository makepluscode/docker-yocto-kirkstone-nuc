import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.2

Dialog {
    id: addDeploymentDialog
    title: "Add New Deployment"
    width: 500
    height: 400
    modal: true
    
    property string name: ""
    property string bundleFile: ""
    property string version: ""
    
    signal deploymentAdded(string name, string bundleFile, string version)
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 15
        
        // Deployment name
        Label {
            text: "Deployment Name:"
            font.pixelSize: 14
            font.bold: true
        }
        
        TextField {
            id: nameField
            Layout.fillWidth: true
            placeholderText: "Enter deployment name"
            text: name
            onTextChanged: name = text
        }
        
        // Bundle file selection
        Label {
            text: "Bundle File:"
            font.pixelSize: 14
            font.bold: true
        }
        
        RowLayout {
            Layout.fillWidth: true
            
            TextField {
                id: bundleField
                Layout.fillWidth: true
                placeholderText: "Select .raucb file"
                text: bundleFile
                readOnly: true
            }
            
            Button {
                text: "Browse"
                onClicked: fileDialog.open()
            }
        }
        
        // Version
        Label {
            text: "Version:"
            font.pixelSize: 14
            font.bold: true
        }
        
        TextField {
            id: versionField
            Layout.fillWidth: true
            placeholderText: "Enter version (e.g., 1.0.0)"
            text: version
            onTextChanged: version = text
        }
        
        Item { Layout.fillHeight: true }
        
        // Buttons
        RowLayout {
            Layout.fillWidth: true
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "Cancel"
                onClicked: addDeploymentDialog.close()
            }
            
            Button {
                text: "Add Deployment"
                enabled: nameField.text.trim() !== "" && bundleField.text !== "" && versionField.text.trim() !== ""
                background: Rectangle {
                    color: parent.enabled ? "#3498db" : "#bdc3c7"
                    radius: 4
                }
                onClicked: {
                    deploymentAdded(nameField.text.trim(), bundleField.text, versionField.text.trim())
                    addDeploymentDialog.close()
                    
                    // Reset fields
                    nameField.text = ""
                    bundleField.text = ""
                    versionField.text = ""
                }
            }
        }
    }
    
    // File dialog
    FileDialog {
        id: fileDialog
        title: "Select Bundle File"
        nameFilters: ["RAUC Bundle Files (*.raucb)", "All Files (*)"]
        onAccepted: {
            bundleField.text = fileDialog.fileUrl.toString().replace("file://", "")
        }
    }
} 