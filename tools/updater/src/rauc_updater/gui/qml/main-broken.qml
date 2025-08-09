import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import RaucUpdater 1.0

ApplicationWindow {
    id: window
    width: 960
    height: 480
    visible: true
    title: qsTr("ARCRO Updater")
    
    property bool updateInProgress: false
    property string selectedPackage: ""
    
    // Color scheme - Raspberry Pi Imager style
    property color backgroundColor: "#f5f5f5"
    property color cardColor: "#ffffff"
    property color primaryColor: "#c51a4a"
    property color secondaryColor: "#8cc04b"
    property color textColor: "#333333"
    property color borderColor: "#e0e0e0"
    
    background: Rectangle {
        color: backgroundColor
    }
    
    // Main content
    Item {
        anchors.fill: parent
        anchors.margins: 40
        
        // Header
        Text {
            id: headerText
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                topMargin: 20
            }
            text: updateInProgress ? qsTr("업데이트 진행 중...") : qsTr("ARCRO Updater")
            font.pixelSize: 24
            font.weight: Font.Bold
            color: textColor
        }
        
        // Main content area
        StackLayout {
            id: stackLayout
            anchors {
                top: headerText.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                topMargin: 30
            }
            currentIndex: updateInProgress ? 1 : 0
            
            // Selection Page
            Item {
                id: selectionPage
                
                // Main horizontal layout
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 20
                    
                    // Left side - Selector cards
                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.maximumWidth: 600
                        spacing: 16
                    
                        // 1st Selector - Computer (Fixed)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            color: cardColor
                            border.color: borderColor
                            border.width: 2
                            radius: 8
                        
                            // Simple shadow using additional rectangle
                            Rectangle {
                                anchors.fill: parent
                                anchors.topMargin: 2
                                anchors.leftMargin: 1
                                color: "#10000000"
                                radius: parent.radius
                                z: -1
                            }
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    color: primaryColor
                                    radius: 20
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "🖥️"
                                        font.pixelSize: 20
                                        color: "white"
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    
                                    Text {
                                        text: qsTr("컴퓨터")
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: qsTr("GUI IPC")
                                        font.pixelSize: 12
                                        color: "#666666"
                                    }
                                }
                                
                                Text {
                                    text: "✓"
                                    font.pixelSize: 20
                                    color: secondaryColor
                                    font.weight: Font.Bold
                                }
                            }
                        }
                    }
                    
                        // 2nd Selector - Update Package
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            color: cardColor
                            border.color: selectedPackage ? secondaryColor : borderColor
                            border.width: 2
                            radius: 8
                        
                        
                            MouseArea {
                                anchors.fill: parent
                                onClicked: fileDialog.open()
                            }
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    color: selectedPackage ? secondaryColor : "#cccccc"
                                    radius: 20
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "📦"
                                        font.pixelSize: 20
                                        color: "white"
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    
                                    Text {
                                        text: qsTr("업데이트 패키지")
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: selectedPackage ? selectedPackage.split('/').pop() : qsTr("RAUC 번들을 선택하세요")
                                        font.pixelSize: 12
                                        color: selectedPackage ? "#666666" : "#999999"
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }
                                
                                Text {
                                    text: selectedPackage ? "✓" : "⚠️"
                                    font.pixelSize: 20
                                    color: selectedPackage ? secondaryColor : "#ff6b35"
                                    font.weight: Font.Bold
                                }
                            }
                        }
                    }
                    
                        // 3rd Selector - Location (Fixed)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            color: cardColor
                            border.color: borderColor
                            border.width: 2
                            radius: 8
                        
                        
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    color: primaryColor
                                    radius: 20
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "💾"
                                        font.pixelSize: 20
                                        color: "white"
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    
                                    Text {
                                        text: qsTr("위치")
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: qsTr("/dev/sdX")
                                        font.pixelSize: 12
                                        color: "#666666"
                                    }
                                }
                                
                                Text {
                                    text: "✓"
                                    font.pixelSize: 20
                                    color: secondaryColor
                                    font.weight: Font.Bold
                                }
                            }
                        }
                    }
                    
                    // Right side - Next button area
                    Item {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 300
                        Layout.minimumWidth: 250
                        
                        Button {
                            id: nextButton
                            anchors.centerIn: parent
                            width: 180
                            height: 50
                            enabled: selectedPackage !== ""
                            
                            background: Rectangle {
                                color: parent.enabled ? primaryColor : "#cccccc"
                                radius: 25
                                
                                Rectangle {
                                    anchors.fill: parent
                                    color: parent.parent.pressed ? "black" : "transparent"
                                    opacity: 0.1
                                    radius: parent.radius
                                }
                            }
                            
                            contentItem: Text {
                                text: qsTr("다음")
                                font.pixelSize: 18
                                font.weight: Font.Bold
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            onClicked: {
                                updateInProgress = true
                                raucController.bundlePath = selectedPackage
                                raucController.startUpdate()
                            }
                        }
                    }
                }
            }
            
            // Progress Page
            Item {
                id: progressPage
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 30
                    width: Math.min(600, parent.width * 0.8)
                    
                    // Progress Info
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("업데이트 중...")
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: textColor
                        }
                        
                        Text {
                            id: progressStatus
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("연결 중...")
                            font.pixelSize: 14
                            color: "#666666"
                        }
                    }
                    
                    // Progress Bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 20
                        color: "#e0e0e0"
                        radius: 10
                        
                        Rectangle {
                            id: progressBar
                            width: parent.width * (raucController.progress / 100)
                            height: parent.height
                            color: secondaryColor
                            radius: parent.radius
                            
                            Behavior on width {
                                PropertyAnimation { duration: 300 }
                            }
                        }
                        
                        Text {
                            anchors.centerIn: parent
                            text: Math.round(raucController.progress) + "%"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            color: textColor
                        }
                    }
                    
                    // Cancel Button
                    Button {
                        id: cancelButton
                        Layout.alignment: Qt.AlignHCenter
                        width: 140
                        height: 45
                        
                        background: Rectangle {
                            color: parent.pressed ? "#d32f2f" : "#f44336"
                            radius: 22
                        }
                        
                        contentItem: Text {
                            text: qsTr("취소")
                            font.pixelSize: 14
                            font.weight: Font.Bold
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            raucController.cancelUpdate()
                            updateInProgress = false
                            selectedPackage = ""
                        }
                    }
                }
            }
        }
    }
    
    // File Dialog
    FileDialog {
        id: fileDialog
        title: qsTr("RAUC 번들 선택")
        nameFilters: ["RAUC bundles (*.raucb)", "All files (*)"]
        onAccepted: {
            selectedPackage = selectedFile.toString().replace("file://", "")
        }
    }
    
    // Success Dialog
    Dialog {
        id: successDialog
        anchors.centerIn: parent
        width: 350
        height: 220
        title: qsTr("업데이트 완료")
        
        Rectangle {
            anchors.fill: parent
            color: cardColor
            radius: 8
            
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                
                Text {
                    text: "✅"
                    font.pixelSize: 48
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: qsTr("업데이트가 성공적으로 완료되었습니다!")
                    font.pixelSize: 14
                    color: textColor
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                }
                
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("확인")
                    onClicked: {
                        successDialog.close()
                        updateInProgress = false
                        selectedPackage = ""
                    }
                    
                    background: Rectangle {
                        color: secondaryColor
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }
    
    // Controller connections
    Connections {
        target: raucController
        function onStatusChanged(status) {
            progressStatus.text = status
        }
        function onUpdateCompleted(success, message) {
            if (success) {
                successDialog.open()
            } else {
                // Handle error
                updateInProgress = false
            }
        }
    }
    
    // Add progress property to controller if not exists
    property int progress: 0
    Connections {
        target: raucController
        function onProgressChanged(value) {
            progress = value
        }
    }
}