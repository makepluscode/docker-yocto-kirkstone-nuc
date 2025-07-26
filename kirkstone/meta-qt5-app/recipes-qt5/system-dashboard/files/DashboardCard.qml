import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: card
    
    property string title: ""
    property alias content: contentItem.children
    default property alias children: contentItem.children
    
    color: "#1a1a1a"
    border.color: "#333333"
    border.width: 1
    radius: 8
    
    Column {
        anchors.fill: parent
        spacing: 0
        
        // Title bar
        Rectangle {
            width: parent.width
            height: 35
            color: "#2a2a2a"
            radius: card.radius
            
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: card.radius
                color: parent.color
            }
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                text: card.title
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
        
        // Content area
        Item {
            id: contentItem
            width: parent.width
            height: parent.height - 35
        }
    }
    
    // Subtle glow effect
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#444444"
        border.width: 1
        radius: card.radius
        opacity: 0.3
    }
} 