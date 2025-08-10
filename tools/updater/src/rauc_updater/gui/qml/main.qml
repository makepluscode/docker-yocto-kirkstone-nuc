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
        anchors.margins: 30
        
        // Header
        Text {
            id: headerText
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                topMargin: 15
            }
            text: updateInProgress ? qsTr("업데이트 진행 중...") : qsTr("ARCRO Updater")
            font.pixelSize: 22
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
                topMargin: 25
            }
            currentIndex: updateInProgress ? 1 : 0
            
            // Selection Page
            Item {
                id: selectionPage
                
                // Horizontal layout for 960x480
                RowLayout {
                    anchors.fill: parent
                    spacing: 30
                    
                    // Left side - Selector cards
                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.maximumWidth: 600
                        spacing: 12
                        
                        // 1st Selector - Computer (Fixed)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 65
                            color: cardColor
                            border.color: borderColor
                            border.width: 2
                            radius: 8
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    color: primaryColor
                                    radius: 18
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "🖥️"
                                        font.pixelSize: 18
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    
                                    Text {
                                        text: qsTr("컴퓨터")
                                        font.pixelSize: 15
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: qsTr("GUI IPC")
                                        font.pixelSize: 11
                                        color: "#666666"
                                    }
                                }
                                
                                Text {
                                    text: "✓"
                                    font.pixelSize: 18
                                    color: secondaryColor
                                    font.weight: Font.Bold
                                }
                            }
                        }
                        
                        // 2nd Selector - Update Package
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 65
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
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    color: selectedPackage ? secondaryColor : "#cccccc"
                                    radius: 18
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "📦"
                                        font.pixelSize: 18
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    
                                    Text {
                                        text: qsTr("업데이트 패키지")
                                        font.pixelSize: 15
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: selectedPackage ? selectedPackage.split('/').pop() : qsTr("RAUC 번들을 선택하세요")
                                        font.pixelSize: 11
                                        color: selectedPackage ? "#666666" : "#999999"
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }
                                
                                Text {
                                    text: selectedPackage ? "✓" : "⚠️"
                                    font.pixelSize: 18
                                    color: selectedPackage ? secondaryColor : "#ff6b35"
                                    font.weight: Font.Bold
                                }
                            }
                        }
                        
                        // 3rd Selector - Location (Fixed)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 65
                            color: cardColor
                            border.color: borderColor
                            border.width: 2
                            radius: 8
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                
                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    color: primaryColor
                                    radius: 18
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "💾"
                                        font.pixelSize: 18
                                    }
                                }
                                
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    
                                    Text {
                                        text: qsTr("위치")
                                        font.pixelSize: 15
                                        font.weight: Font.Bold
                                        color: textColor
                                    }
                                    
                                    Text {
                                        text: qsTr("/dev/sdX")
                                        font.pixelSize: 11
                                        color: "#666666"
                                    }
                                }
                                
                                Text {
                                    text: "✓"
                                    font.pixelSize: 18
                                    color: secondaryColor
                                    font.weight: Font.Bold
                                }
                            }
                        }
                    }
                    
                    // Right side - Next button area
                    Item {
                        Layout.fillHeight: true
                        Layout.preferredWidth: 280
                        Layout.minimumWidth: 220
                        
                        Button {
                            id: nextButton
                            anchors.centerIn: parent
                            width: 160
                            height: 45
                            enabled: selectedPackage !== ""
                            
                            background: Rectangle {
                                color: parent.enabled ? primaryColor : "#cccccc"
                                radius: 22
                                
                                Rectangle {
                                    anchors.fill: parent
                                    color: parent.parent.pressed ? "black" : "transparent"
                                    opacity: 0.1
                                    radius: parent.radius
                                }
                            }
                            
                            contentItem: Text {
                                text: qsTr("다음")
                                font.pixelSize: 16
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
                    spacing: 25
                    width: Math.min(600, parent.width * 0.8)
                    
                    // Progress Info
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("업데이트 중...")
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: textColor
                        }
                        
                        Text {
                            id: progressStatus
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("연결 중...")
                            font.pixelSize: 13
                            color: "#666666"
                        }
                    }
                    
                    // Three-Step Progress Bars
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        // Step 1: Setup (0-25%)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            Text {
                                text: "1. Setup"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: textColor
                                Layout.minimumWidth: 80
                            }
                            
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 12
                                color: "#e0e0e0"
                                radius: 6
                                
                                Rectangle {
                                    width: parent.width * Math.min(1.0, Math.max(0, raucController.progress / 25))
                                    height: parent.height
                                    color: raucController.progress >= 25 ? secondaryColor : "#ff8c00"
                                    radius: parent.radius
                                    
                                    Behavior on width {
                                        PropertyAnimation { duration: 300 }
                                    }
                                }
                            }
                            
                            Text {
                                text: raucController.progress >= 25 ? "✓" : Math.round(Math.min(25, raucController.progress)) + "%"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: raucController.progress >= 25 ? secondaryColor : textColor
                                Layout.minimumWidth: 30
                            }
                        }
                        
                        // Step 2: SW Update (25-85%)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            Text {
                                text: "2. Update"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: raucController.progress >= 25 ? textColor : "#999999"
                                Layout.minimumWidth: 80
                            }
                            
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 12
                                color: "#e0e0e0"
                                radius: 6
                                
                                Rectangle {
                                    width: parent.width * Math.min(1.0, Math.max(0, (raucController.progress - 25) / 60))
                                    height: parent.height
                                    color: raucController.progress >= 85 ? secondaryColor : "#ff8c00"
                                    radius: parent.radius
                                    visible: raucController.progress >= 25
                                    
                                    Behavior on width {
                                        PropertyAnimation { duration: 300 }
                                    }
                                }
                            }
                            
                            Text {
                                text: {
                                    if (raucController.progress < 25) return ""
                                    if (raucController.progress >= 85) return "✓"
                                    return Math.round(Math.min(85, raucController.progress)) + "%"
                                }
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: raucController.progress >= 85 ? secondaryColor : (raucController.progress >= 25 ? textColor : "#999999")
                                Layout.minimumWidth: 30
                            }
                        }
                        
                        // Step 3: Validation (85-100%)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            Text {
                                text: "3. Validate"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: raucController.progress >= 85 ? textColor : "#999999"
                                Layout.minimumWidth: 80
                            }
                            
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 12
                                color: "#e0e0e0"
                                radius: 6
                                
                                Rectangle {
                                    width: parent.width * Math.min(1.0, Math.max(0, (raucController.progress - 85) / 15))
                                    height: parent.height
                                    color: raucController.progress >= 100 ? secondaryColor : "#ff8c00"
                                    radius: parent.radius
                                    visible: raucController.progress >= 85
                                    
                                    Behavior on width {
                                        PropertyAnimation { duration: 300 }
                                    }
                                }
                            }
                            
                            Text {
                                text: {
                                    if (raucController.progress < 85) return ""
                                    if (raucController.progress >= 100) return "✓"
                                    return Math.round(raucController.progress) + "%"
                                }
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                color: raucController.progress >= 100 ? secondaryColor : (raucController.progress >= 85 ? textColor : "#999999")
                                Layout.minimumWidth: 30
                            }
                        }
                        
                        // Overall Progress Summary
                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 3
                            Layout.topMargin: 5
                            
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "전체 진행률: " + Math.round(raucController.progress) + "%"
                                font.pixelSize: 13
                                font.weight: Font.Bold
                                color: textColor
                            }
                            
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: {
                                    var timing = raucController.timingInfo;
                                    if (timing && timing.total_time) {
                                        var totalSecs = Math.round(timing.total_time);
                                        if (totalSecs < 60) {
                                            return "소요 시간: " + totalSecs + "초";
                                        } else {
                                            var mins = Math.floor(totalSecs / 60);
                                            var secs = totalSecs % 60;
                                            return "소요 시간: " + mins + "분 " + secs + "초";
                                        }
                                    }
                                    return "";
                                }
                                font.pixelSize: 11
                                color: "#666666"
                                visible: text !== ""
                            }
                        }
                    }
                    
                    // Cancel Button
                    Button {
                        id: cancelButton
                        Layout.alignment: Qt.AlignHCenter
                        width: 120
                        height: 40
                        
                        background: Rectangle {
                            color: parent.pressed ? "#d32f2f" : "#f44336"
                            radius: 20
                        }
                        
                        contentItem: Text {
                            text: qsTr("취소")
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: {
                            raucController.cancelUpdate()
                            updateInProgress = false
                            selectedPackage = ""
                            raucController.resetState()
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
        width: 420
        height: 320
        title: qsTr("업데이트 완료")
        
        // Timing info property
        property var timingInfo: ({})
        
        function formatTime(seconds) {
            if (seconds < 60) {
                return Math.round(seconds) + "초"
            } else {
                var minutes = Math.floor(seconds / 60)
                var secs = Math.round(seconds % 60)
                return minutes + "분 " + secs + "초"
            }
        }
        
        Rectangle {
            anchors.fill: parent
            color: cardColor
            radius: 8
            
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 15
                width: parent.width - 40
                
                Text {
                    text: "✅"
                    font.pixelSize: 40
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Text {
                    text: qsTr("업데이트가 성공적으로 완료되었습니다!\n장치가 새 이미지로 재부팅되었습니다.")
                    font.pixelSize: 14
                    color: textColor
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
                
                // Timing Information
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    color: "#f8f8f8"
                    radius: 6
                    border.color: "#e0e0e0"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6
                        
                        Text {
                            text: qsTr("단계별 소요 시간")
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            color: textColor
                            Layout.alignment: Qt.AlignHCenter
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "1. Setup:"
                                font.pixelSize: 11
                                color: textColor
                                Layout.preferredWidth: 60
                            }
                            
                            Text {
                                text: successDialog.timingInfo.setup_time ? successDialog.formatTime(successDialog.timingInfo.setup_time) : "0초"
                                font.pixelSize: 11
                                color: secondaryColor
                                font.weight: Font.Bold
                            }
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "2. Update:"
                                font.pixelSize: 11
                                color: textColor
                                Layout.preferredWidth: 60
                            }
                            
                            Text {
                                text: successDialog.timingInfo.update_time ? successDialog.formatTime(successDialog.timingInfo.update_time) : "0초"
                                font.pixelSize: 11
                                color: secondaryColor
                                font.weight: Font.Bold
                            }
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: "3. Validate:"
                                font.pixelSize: 11
                                color: textColor
                                Layout.preferredWidth: 60
                            }
                            
                            Text {
                                text: successDialog.timingInfo.validate_time ? successDialog.formatTime(successDialog.timingInfo.validate_time) : "0초"
                                font.pixelSize: 11
                                color: secondaryColor
                                font.weight: Font.Bold
                            }
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#e0e0e0"
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Text {
                                text: qsTr("총 시간:")
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: textColor
                                Layout.preferredWidth: 60
                            }
                            
                            Text {
                                text: successDialog.timingInfo.total_time ? successDialog.formatTime(successDialog.timingInfo.total_time) : "0초"
                                font.pixelSize: 12
                                color: primaryColor
                                font.weight: Font.Bold
                            }
                        }
                    }
                }
                
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("확인")
                    onClicked: {
                        successDialog.close()
                        updateInProgress = false
                        selectedPackage = ""
                        raucController.resetState()
                        successDialog.timingInfo = {}
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
        function onUpdateCompleted(success, message, timingInfo) {
            if (success) {
                // Store timing info for success dialog
                successDialog.timingInfo = timingInfo
                successDialog.open()
            } else {
                updateInProgress = false
                selectedPackage = ""
                raucController.resetState()
            }
        }
    }
    
    // Progress connections
    Connections {
        target: raucController
        function onProgressChanged(value) {
            // Progress is handled by controller property binding
        }
    }
}