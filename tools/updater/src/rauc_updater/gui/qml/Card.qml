import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    
    property string title: ""
    property string icon: ""
    property alias content: contentLoader.sourceComponent
    
    color: window.cardColor
    border.color: window.darkTheme ? "#555555" : "#e0e0e0"
    border.width: 1
    radius: 8
    
    // Drop shadow effect
    Rectangle {
        id: shadow
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.leftMargin: 2
        color: window.darkTheme ? "#00000040" : "#00000020"
        radius: parent.radius
        z: -1
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // Card header
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: root.icon
                font.pixelSize: 20
                visible: root.icon.length > 0
            }
            
            Label {
                text: root.title
                font.pixelSize: 16
                font.bold: true
                color: window.textColor
                Layout.fillWidth: true
            }
            
            // Optional header controls can be added here
        }
        
        // Separator line
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: window.darkTheme ? "#555555" : "#e0e0e0"
            visible: root.title.length > 0
        }
        
        // Card content
        Loader {
            id: contentLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}