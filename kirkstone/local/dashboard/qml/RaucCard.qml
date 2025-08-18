import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Rauc 1.0

Item {
    id: root
    property alias manager: raucManager

    width: parent.width

    Column {
        spacing: 10
        anchors.margins: 12
        anchors.fill: parent

        Text {
            text: "RAUC Status"
            font.pixelSize: 24
            font.bold: true
        }

        GridLayout {
            columns: 2
            rowSpacing: 6
            columnSpacing: 20

            Text { text: "Booted Slot:"; color: "#cccccc" }
            Text { text: manager.bootSlot; color: "#ffffff" }

            Text { text: "Activated Slot:"; color: "#cccccc" }
            Text { text: manager.activatedSlot; color: "#ffffff" }
        }

        Rectangle { height: 1; width: parent.width; color: "#444" }

        ScrollView {
            height: 160
            TextArea {
                width: parent.width
                text: manager.statusText
                readOnly: true
                wrapMode: TextArea.Wrap
            }
        }

        Row {
            spacing: 12
            Button { text: "Boot Slot A"; onClicked: manager.bootSlotA() }
            Button { text: "Boot Slot B"; onClicked: manager.bootSlotB() }
            Button { text: "Refresh";   onClicked: manager.refresh() }
        }
    }

    RaucManager { id: raucManager }

    Component.onCompleted: raucManager.refresh()
}
