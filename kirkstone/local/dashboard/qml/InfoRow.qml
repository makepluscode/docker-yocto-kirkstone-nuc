import QtQuick 2.15

Row {
    property string label: ""
    property string value: ""
    
    spacing: 10
    
    Text {
        text: label
        color: "#cccccc"
        font.pointSize: 10
        width: 100
        horizontalAlignment: Text.AlignRight
        anchors.verticalCenter: parent.verticalCenter
    }
    
    Text {
        text: value
        color: "#ffffff"
        font.pointSize: 10
        font.bold: true
        anchors.verticalCenter: parent.verticalCenter
    }
} 