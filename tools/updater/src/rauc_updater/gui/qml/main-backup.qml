import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import RaucUpdater 1.0

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("ARCRO - Advanced RAUC Control & Rollout Operations")
    
    property bool darkTheme: false
    
    // Color scheme
    property color backgroundColor: darkTheme ? "#2b2b2b" : "#f5f5f5"
    property color cardColor: darkTheme ? "#3c3c3c" : "#ffffff"
    property color textColor: darkTheme ? "#ffffff" : "#333333"
    property color accentColor: "#0078d4"
    property color successColor: "#107c10"
    property color errorColor: "#d13438"
    property color warningColor: "#ff8c00"
    
    color: backgroundColor
    
    // Header bar
    header: ToolBar {
        background: Rectangle {
            color: accentColor
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Label {
                text: qsTr("RAUC Updater")
                color: "white"
                font.pixelSize: 18
                font.bold: true
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Button {
                text: darkTheme ? qsTr("☀") : qsTr("🌙")
                flat: true
                onClicked: darkTheme = !darkTheme
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Toggle theme")
            }
        }
    }
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        
        ColumnLayout {
            width: window.width - 40
            spacing: 20
            
            // Connection Settings Card
            ConnectionCard {
                id: connectionCard
                Layout.fillWidth: true
                controller: raucController
            }
            
            // Bundle Selection Card  
            BundleCard {
                id: bundleCard
                Layout.fillWidth: true
                controller: raucController
                enabled: raucController.isConnected
            }
            
            // Progress Card
            ProgressCard {
                id: progressCard
                Layout.fillWidth: true
                controller: raucController
                visible: raucController.isUpdating
            }
            
            // Log Viewer Card
            LogCard {
                id: logCard
                Layout.fillWidth: true
                Layout.fillHeight: true
                controller: raucController
            }
        }
    }
    
    // Status bar
    footer: ToolBar {
        background: Rectangle {
            color: cardColor
            border.color: darkTheme ? "#555555" : "#dddddd"
            border.width: 1
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: raucController.isConnected ? successColor : errorColor
            }
            
            Label {
                text: raucController.isConnected ? qsTr("Connected") : qsTr("Disconnected")
                color: textColor
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Label {
                text: qsTr("Ready")
                color: textColor
                font.pixelSize: 12
            }
        }
    }
}