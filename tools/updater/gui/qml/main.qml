import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Window 6.0

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: "Updater Server - Management Console"
    
    // Header
    header: ToolBar {
        background: Rectangle {
            color: "#2c3e50"
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Text {
                text: "🔄 Updater Server Management Console"
                color: "white"
                font.pixelSize: 18
                font.bold: true
            }
            
            Item { Layout.fillWidth: true }
            
            Row {
                spacing: 10
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: "#27ae60"
                }
                
                Text {
                    text: "Connected"
                    color: "white"
                }
                
                Button {
                    text: "Refresh"
                    onClicked: {
                        console.log("Refresh clicked")
                        statusText.text = "Status updated at " + new Date().toLocaleTimeString()
                    }
                }
            }
        }
    }
    
    // Main content
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        // Status Cards Row
        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            
            // Server Status Card
            Rectangle {
                Layout.fillWidth: true
                height: 120
                color: "#ecf0f1"
                border.color: "#bdc3c7"
                border.width: 1
                radius: 8
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: "🖥️ Server Status"
                        font.bold: true
                        font.pixelSize: 16
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Status:" }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: 60
                            height: 20
                            radius: 10
                            color: "#27ae60"
                            Text {
                                anchors.centerIn: parent
                                text: "Running"
                                color: "white"
                                font.pixelSize: 12
                            }
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Version:" }
                        Item { Layout.fillWidth: true }
                        Text { text: "0.2.0"; font.bold: true }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Protocol:" }
                        Item { Layout.fillWidth: true }
                        Text { text: "HTTPS"; font.bold: true }
                    }
                }
            }
            
            // Deployments Card
            Rectangle {
                Layout.fillWidth: true
                height: 120
                color: "#ecf0f1"
                border.color: "#bdc3c7"
                border.width: 1
                radius: 8
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: "📦 Active Deployments"
                        font.bold: true
                        font.pixelSize: 16
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total:" }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: 30
                            height: 20
                            radius: 10
                            color: "#3498db"
                            Text {
                                anchors.centerIn: parent
                                text: "1"
                                color: "white"
                                font.pixelSize: 12
                            }
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Last Updated:" }
                        Item { Layout.fillWidth: true }
                        Text { 
                            id: statusText
                            text: "Now"
                            font.pixelSize: 12 
                        }
                    }
                }
            }
            
            // Bundle Storage Card
            Rectangle {
                Layout.fillWidth: true
                height: 120
                color: "#ecf0f1"
                border.color: "#bdc3c7"
                border.width: 1
                radius: 8
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: "💾 Bundle Storage"
                        font.bold: true
                        font.pixelSize: 16
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Files:" }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: 30
                            height: 20
                            radius: 10
                            color: "#17a2b8"
                            Text {
                                anchors.centerIn: parent
                                text: "1"
                                color: "white"
                                font.pixelSize: 12
                            }
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Total Size:" }
                        Item { Layout.fillWidth: true }
                        Text { text: "152 MB"; font.pixelSize: 12 }
                    }
                }
            }
        }
        
        // Main Content Area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20
            
            // Deployments Table
            GroupBox {
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: "Deployments Management"
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10
                    
                    // Toolbar
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Text {
                            text: "📋 Deployments"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Button {
                            text: "Upload Bundle"
                            onClicked: console.log("Upload bundle clicked")
                        }
                        
                        Button {
                            text: "New Deployment"
                            onClicked: console.log("New deployment clicked")
                        }
                    }
                    
                    // Simple deployment list
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#f8f9fa"
                        border.color: "#dee2e6"
                        border.width: 1
                        
                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 10
                            
                            Column {
                                width: parent.width
                                spacing: 5
                                
                                // Header
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: "#34495e"
                                    
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 20
                                        
                                        Text { text: "Execution ID"; color: "white"; width: 150 }
                                        Text { text: "Version"; color: "white"; width: 100 }
                                        Text { text: "Filename"; color: "white"; width: 200 }
                                        Text { text: "Size"; color: "white"; width: 80 }
                                        Text { text: "Status"; color: "white"; width: 80 }
                                    }
                                }
                                
                                // Sample deployment row
                                Rectangle {
                                    width: parent.width
                                    height: 50
                                    color: "#ffffff"
                                    border.color: "#bdc3c7"
                                    border.width: 1
                                    
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 20
                                        
                                        Text { text: "exec-nuc-image-qt5"; width: 150; font.family: "monospace" }
                                        Text { text: "bundle"; width: 100; font.bold: true }
                                        Text { text: "nuc-image-qt5-bundle-intel-corei7-64.raucb"; width: 200 }
                                        Text { text: "152 MB"; width: 80 }
                                        Rectangle {
                                            width: 60; height: 20; radius: 10
                                            color: "#27ae60"
                                            Text {
                                                anchors.centerIn: parent
                                                text: "Active"
                                                color: "white"
                                                font.pixelSize: 11
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Activity Log
            GroupBox {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                title: "Activity Log"
                
                ScrollView {
                    anchors.fill: parent
                    
                    Rectangle {
                        width: parent.width
                        height: Math.max(parent.height, logColumn.implicitHeight)
                        color: "#f8f9fa"
                        
                        Column {
                            id: logColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5
                            
                            Text {
                                text: "[" + new Date().toLocaleTimeString() + "] Server started"
                                font.family: "monospace"
                                font.pixelSize: 12
                                width: parent.width
                                wrapMode: Text.Wrap
                            }
                            
                            Text {
                                text: "[" + new Date().toLocaleTimeString() + "] Bundle loaded: nuc-image-qt5-bundle"
                                font.family: "monospace"
                                font.pixelSize: 12
                                width: parent.width
                                wrapMode: Text.Wrap
                            }
                            
                            Text {
                                text: "[" + new Date().toLocaleTimeString() + "] HTTPS enabled on port 8443"
                                font.family: "monospace"
                                font.pixelSize: 12
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: "#27ae60"
                            }
                            
                            Text {
                                text: "[" + new Date().toLocaleTimeString() + "] GUI connected successfully"
                                font.family: "monospace"
                                font.pixelSize: 12
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: "#3498db"
                            }
                        }
                    }
                }
            }
        }
    }
}