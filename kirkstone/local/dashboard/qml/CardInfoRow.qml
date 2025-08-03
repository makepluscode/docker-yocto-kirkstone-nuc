import QtQuick 2.15

Row {
    id: infoRow
    
    property string label: ""
    property string value: ""
    property int labelWidth: 100
    property int labelFontSize: 10
    property int valueFontSize: 10
    property color labelColor: "#cccccc"
    property color valueColor: "#ffffff"
    property bool valueBold: true
    
    spacing: 10
    
    Text {
        text: label + ":"
        color: labelColor
        font.pointSize: labelFontSize
        width: labelWidth
        horizontalAlignment: Text.AlignRight
    }
    
    Text {
        text: value
        color: valueColor
        font.pointSize: valueFontSize
        font.bold: valueBold
        elide: Text.ElideRight
        width: Math.max(infoRow.parent ? infoRow.parent.width - labelWidth - spacing - 20 : 200, 50)
    }
}