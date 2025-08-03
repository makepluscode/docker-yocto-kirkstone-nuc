import QtQuick 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: cardBase
    
    // Public properties
    property string title: ""
    property bool isEmpty: false
    property string emptyText: "Empty"
    property var cardData: null
    
    // Default content properties - can be overridden
    default property alias content: contentArea.children
    
    // Card styling - consistent across all cards
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: isEmpty ? "#0f0f0f" : "#1a1a1a"
    border.color: isEmpty ? "#222222" : "#444444"
    border.width: isEmpty ? 1 : 2
    radius: 8
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // Title bar
        Rectangle {
            id: titleBar
            width: parent.width
            height: 35
            color: isEmpty ? "#1a1a1a" : "#2a2a2a"
            radius: cardBase.radius
            
            // Bottom corners should be square
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: cardBase.radius
                color: parent.color
            }
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                text: cardBase.title
                color: isEmpty ? "#666666" : "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
        
        // Content area
        Item {
            id: contentArea
            width: parent.width
            height: parent.height - titleBar.height
            
            // Default empty content
            Text {
                anchors.centerIn: parent
                text: cardBase.emptyText
                color: "#444444"
                font.pointSize: 14
                visible: isEmpty || contentArea.children.length === 1 // Only this text
            }
        }
    }
    
    // Subtle glow effect for active cards
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: isEmpty ? "transparent" : "#555555"
        border.width: 1
        radius: cardBase.radius
        opacity: 0.3
        visible: !isEmpty
    }
}