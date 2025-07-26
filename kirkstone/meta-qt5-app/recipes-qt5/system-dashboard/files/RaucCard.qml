import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property alias manager: raucManager

    width: parent.width
    Column {
        spacing: 8
        anchors.margins: 12
        anchors.fill: parent

        Text {
            text: "RAUC Status"
            font.pixelSize: 24
            font.bold: true
        }

        ScrollView {
            height: 200
            TextArea {
                text: manager.statusText
                readOnly: true
                wrapMode: TextArea.Wrap
            }
        }

        Row {
            spacing: 12
            Button {
                text: "Boot Slot A"
                onClicked: manager.bootSlotA()
            }
            Button {
                text: "Boot Slot B"
                onClicked: manager.bootSlotB()
            }
            Button {
                text: "Refresh"
                onClicked: manager.refresh()
            }
        }
    }

    RaucManager { id: raucManager }
} 