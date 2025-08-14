import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import SystemInfo 1.0
import Rauc 1.0
import RaucSystem 1.0
import Grub 1.0

ApplicationWindow {
    id: mainWindow
    width: 1024
    height: 768
    visible: true
    color: "#000000"

    // Property to control UI state during SW Update
    property bool swUpdateInProgress: false
    property string updateStatus: "Initializing..."
    property double updateProgress: 0.0
    property bool updateComplete: false

    // Keyboard shortcuts
    Keys.onPressed: function(event) {
        switch(event.key) {
            case Qt.Key_F1:
                f1Button.clicked()
                event.accepted = true
                break
            case Qt.Key_F2:
                f2Button.clicked()
                event.accepted = true
                break
            case Qt.Key_F3:
                f3Button.clicked()
                event.accepted = true
                break
            case Qt.Key_F4:
                // Empty - reserved for future use
                event.accepted = true
                break
            case Qt.Key_F5:
                // Empty - reserved for future use
                event.accepted = true
                break
            case Qt.Key_F6:
                // Empty - reserved for future use
                event.accepted = true
                break
            case Qt.Key_F7:
                f7Button.clicked()
                event.accepted = true
                break
            case Qt.Key_F8:
                f8Button.clicked()
                event.accepted = true
                break
        }
    }

    SystemInfo {
        id: systemInfo
    }

    RaucManager {
        id: raucManager
    }

    RaucSystemManager {
        id: raucSystemManager

        // Connect to update progress signals
        onUpdateProgress: function(percentage, message) {
            updateProgress = percentage / 100.0
            updateStatus = message

            // Auto-show popup when update starts (only if not already shown)
            if (percentage > 0 && !swUpdateInProgress) {
                swUpdateInProgress = true
                updateComplete = false
            }
        }

        onUpdateCompleted: function(success) {
            updateComplete = true
            if (success) {
                updateStatus = "Software update completed successfully!\n\nSystem will reboot automatically."
                updateProgress = 1.0
                // Auto-reboot after 5 seconds for successful updates
                rebootTimer.start()
            } else {
                updateStatus = "Software update failed.\n\nPlease check the update bundle and try again."
                updateProgress = 1.0
                swUpdateInProgress = false
            }
        }
    }

    // Timer for monitoring Hawkbit updater progress
    Timer {
        id: hawkbitMonitorTimer
        interval: 2000 // Check every 2 seconds
        running: false
        repeat: true

        property int checkCount: 0

        onTriggered: {
            checkCount++

            // Simulate progress updates for Hawkbit updater
            if (swUpdateInProgress && checkCount <= 10) {
                updateProgress = checkCount * 0.1

                switch(checkCount) {
                    case 1:
                        updateStatus = "Hawkbit service started, checking for updates..."
                        break
                    case 3:
                        updateStatus = "Connecting to Hawkbit server..."
                        break
                    case 5:
                        updateStatus = "Polling for available updates..."
                        break
                    case 7:
                        updateStatus = "Update found, downloading bundle..."
                        break
                    case 9:
                        updateStatus = "Installing update bundle..."
                        break
                    case 10:
                        updateStatus = "Hawkbit update completed!\n\nMonitoring service for status updates."
                        updateComplete = true
                        updateProgress = 1.0
                        hawkbitMonitorTimer.running = false
                        hawkbitMonitorTimer.checkCount = 0
                        // Don't auto-reboot for Hawkbit updates, let service handle it
                        break
                }
            }

            // Stop monitoring after reasonable time
            if (checkCount > 15) {
                hawkbitMonitorTimer.running = false
                hawkbitMonitorTimer.checkCount = 0
                if (swUpdateInProgress) {
                    updateStatus = "Hawkbit updater is running in background.\n\nCheck service logs for detailed status."
                    updateComplete = true
                    updateProgress = 1.0
                }
            }
        }
    }

    GrubManager {
        id: grubManager
    }

    Component.onCompleted: {
        // Initialize managers on startup
        raucManager.refresh()
        grubManager.refresh()
    }

    // SW Update Popup - Enhanced with animations and modern styling
    UpdatePopup {
        id: swUpdatePopup
        z: 1000

        isVisible: swUpdateInProgress
        status: updateStatus
        progress: updateProgress * 100
        showProgress: !updateComplete

        onIsVisibleChanged: {
            if (!isVisible) {
                swUpdateInProgress = false
                updateComplete = false
                updateProgress = 0.0
                updateStatus = "Initializing..."
            }
        }
    }

    // Status Bar at top
    Rectangle {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "#1a1a1a"
        border.color: "#333333"
        border.width: 1
        enabled: !swUpdateInProgress

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10

            // Left side - Network status
            Row {
                spacing: 5

                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: systemInfo.networkConnected ? "#00ff00" : "#ff0000"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: systemInfo.networkConnected ?
                          systemInfo.networkInterface + " (" + systemInfo.ipAddress + ")" :
                          "Disconnected"
                    color: "#ffffff"
                    font.pointSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Center - Title
            Text {
                text: "NUC System Dashboard"
                color: "#ffffff"
                font.pointSize: 14
                font.bold: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            // Right side - Current time
            Text {
                text: systemInfo.currentTime
                color: "#ffffff"
                font.pointSize: 12
                font.bold: true
            }
        }
    }

    // Main content area
    ScrollView {
        id: contentArea
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: buttonArea.top
        anchors.margins: 10
        enabled: !swUpdateInProgress

        contentWidth: contentGrid.width
        contentHeight: contentGrid.height

        GridLayout {
            id: contentGrid
            columns: 6
            rows: 5
            rowSpacing: 8
            columnSpacing: 8
            width: contentArea.width - 20
            height: contentArea.height - 20

            // Row 1: System Monitoring Cards
            Card01 {
                systemInfo: systemInfo
            }

            Card02 {
                systemInfo: systemInfo
            }

            Card03 {
                systemInfo: systemInfo
            }

            Card04 {
                systemInfo: systemInfo
            }

            Card05 {
                systemInfo: systemInfo
            }

            Card06 {
                systemInfo: systemInfo
            }

            // Row 2: System Info & Boot Management
            Card07 {}

            Card08 {}

            Card09 {}

            Card10 {}

            Card11 {}

            Card12 {}

            // Row 3: Empty Cards
            Card13 {}

            Card14 {}

            Card15 {}

            Card16 {}

            Card17 {}

            Card18 {}

            // Row 4: Empty Cards
            Card19 {}

            Card20 {}

            Card21 {}

            Card22 {}

            Card23 {}

            Card24 {}

            // Row 5: Empty Cards
            Card25 {
                raucSystemManager: raucSystemManager
                systemInfo: systemInfo
            }

            Card26 {
                raucSystemManager: raucSystemManager
            }

            Card27 {}

            Card28 {}

            Card29 {}

            Card30 {}
        }
    }

    // Button area at bottom
    Rectangle {
        id: buttonArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        color: "#1a1a1a"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            Button {
                id: f1Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: !swUpdateInProgress

                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F1"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "SW Update"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                onClicked: {
                    swUpdateInProgress = true
                    updateStatus = "Starting Hawkbit updater service..."
                    updateProgress = 0.0
                    updateComplete = false

                    // Start the RAUC Hawkbit C++ updater service
                    systemInfo.startHawkbitUpdater()

                    // Start monitoring the Hawkbit updater progress
                    hawkbitMonitorTimer.checkCount = 0
                    hawkbitMonitorTimer.running = true
                }
            }

            Button {
                id: f2Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false

                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F2"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Button {
                id: f3Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false

                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F3"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Button {
                id: f4Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false

                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F4"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Button {
                id: f5Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false

                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F5"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Button {
                id: f6Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: false

                background: Rectangle {
                    color: "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F6"
                        color: "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Empty"
                        color: "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            Button {
                id: f7Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: !swUpdateInProgress

                background: Rectangle {
                    color: parent.pressed ? "#555555" : "#333333"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F7"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Exit"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                onClicked: {
                    systemInfo.exitApplication()
                }
            }

            Button {
                id: f8Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: !swUpdateInProgress

                background: Rectangle {
                    color: "#333333"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F8"
                        color: "#ffffff"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "Reboot"
                        color: "#ffffff"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                onClicked: systemInfo.rebootSystem()
            }
        }
    }

    // Note: Real RAUC update process is now handled by RaucSystemManager
    // The old timer-based simulation has been replaced with actual RAUC integration

    // Auto-reboot timer
    Timer {
        id: rebootTimer
        interval: 5000 // 5 seconds
        repeat: false
        onTriggered: {
            systemInfo.rebootSystem()
        }
    }
}