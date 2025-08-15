
import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 600
    height: 400
    title: "RAUC Updater - Minimal Test"
    
    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"
        
        Column {
            anchors.centerIn: parent
            spacing: 20
            
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "RAUC Updater GUI Test"
                font.pixelSize: 24
                color: "#333333"
            }
            
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Test Button"
                onClicked: console.log("Button clicked!")
            }
        }
    }
}
