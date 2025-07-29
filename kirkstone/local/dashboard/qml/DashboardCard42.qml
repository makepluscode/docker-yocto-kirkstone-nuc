import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "#0f0f0f"
    border.color: "#222222"
    border.width: 1
    radius: 8

    Column {
        anchors.fill: parent
        spacing: 0

        // Title bar
        Rectangle {
            width: parent.width
            height: 35
            color: "#1a1a1a"
            radius: 8

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                text: "Empty Card"
                color: "#666666"
                font.pointSize: 12
                font.bold: true
            }
        }

        // Content
        Text {
            anchors.centerIn: parent
            text: "Empty"
            color: "#444444"
            font.pointSize: 14
        }
    }
} 