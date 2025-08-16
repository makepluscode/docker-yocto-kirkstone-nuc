import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import SystemInfo 1.0
import Rauc 1.0
import RaucSystem 1.0
import Grub 1.0
import UpdateAgent 1.0

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
                if (f2Button.enabled) {
                    f2Button.clicked()
                }
                event.accepted = true
                break
            case Qt.Key_F3:
                if (f3Button.enabled) {
                    f3Button.clicked()
                }
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
            case Qt.Key_F9:
                // Test Update Agent popup functionality
                systemInfo.logUIEvent("F9 pressed", "Testing Update Agent popup")
                if (updateAgentManager) {
                    updateAgentManager.testProgressParsing("Update started: downloading bundle Progress: 25%")
                }
                event.accepted = true
                break
            case Qt.Key_F10:
                // Toggle popup for testing
                swUpdateInProgress = !swUpdateInProgress
                if (swUpdateInProgress) {
                    updateStatus = "Testing popup functionality..."
                    updateProgress = 0.5
                    updateComplete = false
                    systemInfo.logUIEvent("F10 pressed", "Manual popup test triggered")
                }
                event.accepted = true
                break
        }
    }

    SystemInfo {
        id: systemInfo
        
        // Connect to Hawkbit service status signals
        onHawkbitServiceStatusChanged: function(active) {
            systemInfo.logUIEvent("Hawkbit service status changed", "Active: " + active)
            
            if (!active && swUpdateInProgress) {
                // Service stopped unexpectedly during update
                systemInfo.logUIEvent("Hawkbit service failure", "Service stopped during update")
                updateStatus = "Hawkbit service stopped unexpectedly"
                updateComplete = true
                updateProgress = 0.0
                swUpdateInProgress = false
                hawkbitMonitorTimer.running = false
                hawkbitMonitorTimer.checkCount = 0
            }
        }
        
        onHawkbitUpdateFailed: function(error) {
            systemInfo.logUIEvent("Hawkbit update failed", "Error: " + error)
            updateStatus = "Update failed: " + error
            updateComplete = true
            updateProgress = 0.0
            swUpdateInProgress = false
            hawkbitMonitorTimer.running = false
            hawkbitMonitorTimer.checkCount = 0
        }
    }

    RaucManager {
        id: raucManager
    }

    RaucSystemManager {
        id: raucSystemManager

        // Connect to update progress signals from RAUC
        onUpdateProgress: function(percentage, message) {
            // Update UI with RAUC progress (both manual and Hawkbit-triggered)
            if (message.includes("RAUC")) {
                systemInfo.logUIEvent("RAUC update progress", "Progress: " + percentage + "% - " + message)
                updateProgress = percentage / 100.0
                updateStatus = message

                // Auto-show popup when RAUC update starts
                if (percentage > 0 && !swUpdateInProgress) {
                    systemInfo.logUIEvent("RAUC update started", "Triggered from D-Bus monitoring")
                    swUpdateInProgress = true
                    updateComplete = false
                }
            }
        }

        onUpdateCompleted: function(success) {
            systemInfo.logUIEvent("RAUC update completed", "Success: " + success)
            updateComplete = true
            if (success) {
                systemInfo.logUIEvent("RAUC update successful", "Auto-reboot will start in 5 seconds")
                updateStatus = "RAUC update completed successfully!\n\nSystem will reboot to new slot."
                updateProgress = 1.0
                // Auto-reboot after 5 seconds for successful RAUC updates
                rebootTimer.start()
            } else {
                systemInfo.logUIEvent("RAUC update failed", "Update bundle installation failed")
                updateStatus = "RAUC update failed.\n\nPlease check the update bundle and try again."
                updateProgress = 1.0
                swUpdateInProgress = false
            }
            
            // Stop Hawkbit monitoring if it was running
            if (hawkbitMonitorTimer.running) {
                hawkbitMonitorTimer.running = false
                hawkbitMonitorTimer.checkCount = 0
            }
        }
    }

    // Timer for monitoring Hawkbit service status
    Timer {
        id: hawkbitMonitorTimer
        interval: 3000 // Check every 3 seconds
        running: false
        repeat: true

        property int checkCount: 0
        property int maxChecks: 60 // 3 minutes timeout (60 * 3 seconds)

        onTriggered: {
            checkCount++

            if (swUpdateInProgress) {
                // Check actual service status
                var serviceActive = systemInfo.checkHawkbitServiceStatus()
                
                if (serviceActive) {
                    // Service is running, check logs for progress
                    var logs = systemInfo.getHawkbitServiceLogs(10)
                    
                    // Parse logs to determine actual status
                    if (logs.includes("Connected to server") || logs.includes("connection established")) {
                        updateStatus = "Connected to Hawkbit server, polling for updates..."
                        updateProgress = 0.2
                    } else if (logs.includes("Deployment found") || logs.includes("download")) {
                        updateStatus = "Update available, downloading..."
                        updateProgress = 0.4
                    } else if (logs.includes("Installing") || logs.includes("rauc install")) {
                        updateStatus = "Installing update bundle via RAUC..."
                        updateProgress = 0.7
                        
                        // Start RAUC D-Bus monitoring when installation begins
                        systemInfo.logUIEvent("RAUC installation detected", "Starting D-Bus monitoring, stopping Hawkbit monitoring")
                        raucSystemManager.monitorRaucDBus()
                        
                        // Stop Hawkbit monitoring, let RAUC monitoring take over
                        hawkbitMonitorTimer.running = false
                        hawkbitMonitorTimer.checkCount = 0
                    } else if (logs.includes("Installation completed") || logs.includes("success")) {
                        updateStatus = "Update installation completed successfully!"
                        updateComplete = true
                        updateProgress = 1.0
                        hawkbitMonitorTimer.running = false
                        hawkbitMonitorTimer.checkCount = 0
                    } else if (logs.includes("error") || logs.includes("failed")) {
                        updateStatus = "Update failed. Check service logs for details."
                        updateComplete = true
                        updateProgress = 0.0
                        hawkbitMonitorTimer.running = false
                        hawkbitMonitorTimer.checkCount = 0
                        swUpdateInProgress = false
                    } else {
                        // Still starting up or waiting
                        updateStatus = "Hawkbit service active, waiting for server response..."
                        updateProgress = 0.1
                    }
                } else {
                    // Service stopped or failed
                    updateStatus = "Hawkbit service is not active. Update failed."
                    updateComplete = true
                    updateProgress = 0.0
                    hawkbitMonitorTimer.running = false
                    hawkbitMonitorTimer.checkCount = 0
                    swUpdateInProgress = false
                }
            }

            // Timeout after maximum checks
            if (checkCount >= maxChecks) {
                systemInfo.logUIEvent("Hawkbit monitoring timeout", "Stopped monitoring after " + maxChecks + " checks (3 minutes)")
                hawkbitMonitorTimer.running = false
                hawkbitMonitorTimer.checkCount = 0
                if (swUpdateInProgress) {
                    updateStatus = "Update timeout. Service may still be running in background.\n\nCheck service logs: journalctl -u rauc-hawkbit-cpp"
                    updateComplete = true
                    updateProgress = 0.0
                    swUpdateInProgress = false
                }
            }
        }
    }

    GrubManager {
        id: grubManager
    }

    UpdateAgentManager {
        id: updateAgentManager
        
        onUpdateStarted: {
            systemInfo.logUIEvent("Update Agent", "Update started")
            swUpdateInProgress = true
            updateComplete = false
            updateStatus = "Update-agent starting update process..."
            updateProgress = 0.0
        }
        
        onUpdateStatusChanged: {
            systemInfo.logUIEvent("Update Agent Status", updateAgentManager.updateStatus)
            updateStatus = updateAgentManager.updateStatus
            if (updateAgentManager.isUpdateActive) {
                swUpdateInProgress = true
                updateComplete = false
            }
        }
        
        onUpdateProgressChanged: {
            systemInfo.logUIEvent("Update Agent Progress", "Progress: " + updateAgentManager.updateProgress + "%")
            if (updateAgentManager.isUpdateActive) {
                updateProgress = updateAgentManager.updateProgress / 100.0
                swUpdateInProgress = true
                updateComplete = false
            }
        }
        
        onUpdateCompleted: function(success, message) {
            systemInfo.logUIEvent("Update Agent Completed", "Success: " + success + " - " + message)
            updateComplete = true
            swUpdateInProgress = false
            updateStatus = success ? "Update completed successfully!" : "Update failed: " + message
            updateProgress = success ? 1.0 : 0.0
            
            if (success) {
                // Auto-reboot after successful update
                rebootTimer.start()
            }
        }
    }

    Component.onCompleted: {
        systemInfo.logUIEvent("Dashboard startup", "Initializing managers")
        
        // Initialize managers on startup
        raucManager.refresh()
        grubManager.refresh()
        updateAgentManager.refresh()
        
        systemInfo.logUIEvent("Dashboard initialized", "All components loaded")
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
            Card07 {
                systemInfo: systemInfo
            }

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
                updateAgentManager: updateAgentManager
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
                    systemInfo.logUIEvent("F1 button clicked", "Software update initiated by user")
                    
                    swUpdateInProgress = true
                    updateStatus = "Running pre-flight checks..."
                    updateProgress = 0.0
                    updateComplete = false

                    // Pre-flight checks
                    systemInfo.logUIEvent("Pre-flight check", "Testing network connectivity")
                    var networkOk = systemInfo.testNetworkConnectivity()
                    
                    if (networkOk) {
                        updateStatus = "Network OK, checking Hawkbit configuration..."
                        systemInfo.logUIEvent("Pre-flight check", "Network connectivity passed")
                        
                        var configStatus = systemInfo.checkHawkbitConfiguration()
                        systemInfo.logUIEvent("Configuration check", "Config validation completed")
                        
                        if (configStatus.includes("hawkbit_server")) {
                            updateStatus = "Configuration OK, starting Hawkbit service..."
                            // Start the RAUC Hawkbit C++ updater service
                            systemInfo.startHawkbitUpdater()

                            // Start monitoring the Hawkbit updater progress
                            hawkbitMonitorTimer.checkCount = 0
                            hawkbitMonitorTimer.running = true
                        } else {
                            systemInfo.logUIEvent("Pre-flight failed", "No Hawkbit server configuration found")
                            updateStatus = "Configuration Error: No Hawkbit server configured!\n\nCheck /etc/rauc-hawkbit-cpp/config.json"
                            updateComplete = true
                            swUpdateInProgress = false
                        }
                    } else {
                        systemInfo.logUIEvent("Pre-flight failed", "Network connectivity test failed")
                        updateStatus = "Network Error: Cannot reach internet!\n\nCheck network connection"
                        updateComplete = true
                        swUpdateInProgress = false
                    }
                }
            }

            Button {
                id: f2Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: swUpdateInProgress

                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? "#aa0000" : "#ff0000") : "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F2"
                        color: parent.parent.enabled ? "#ffffff" : "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: parent.parent.enabled ? "Stop Update" : "Empty"
                        color: parent.parent.enabled ? "#ffffff" : "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                onClicked: {
                    systemInfo.logUIEvent("F2 button clicked", "Software update cancellation requested by user")
                    
                    // Stop the Hawkbit update process
                    systemInfo.stopHawkbitUpdater()
                    
                    // Stop monitoring and reset UI
                    hawkbitMonitorTimer.running = false
                    hawkbitMonitorTimer.checkCount = 0
                    swUpdateInProgress = false
                    updateComplete = true
                    updateStatus = "Update cancelled by user"
                    updateProgress = 0.0
                }
            }

            Button {
                id: f3Button
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                enabled: !swUpdateInProgress

                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? "#555555" : "#333333") : "#1a1a1a"
                    radius: 5
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    Text {
                        text: "F3"
                        color: parent.parent.enabled ? "#ffffff" : "#666666"
                        font.pointSize: 10
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: parent.parent.enabled ? "Diagnose" : "Diagnose"
                        color: parent.parent.enabled ? "#ffffff" : "#666666"
                        font.pointSize: 8
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                
                onClicked: {
                    systemInfo.logUIEvent("F3 button clicked", "Hawkbit diagnostics requested by user")
                    
                    // Show diagnostic info in update popup
                    swUpdateInProgress = true
                    updateComplete = false
                    updateProgress = 0.0
                    
                    var diagResult = "=== HAWKBIT DIAGNOSTICS ===\n\n"
                    
                    diagResult += "1. Network Test:\n"
                    var networkOk = systemInfo.testNetworkConnectivity()
                    diagResult += networkOk ? "✓ PASS - Internet reachable\n\n" : "✗ FAIL - No internet access\n\n"
                    
                    diagResult += "2. Configuration Check:\n"
                    var configInfo = systemInfo.checkHawkbitConfiguration()
                    if (configInfo.includes("hawkbit_server")) {
                        diagResult += "✓ PASS - Server configured\n\n"
                    } else {
                        diagResult += "✗ FAIL - No server config\n\n"
                    }
                    
                    diagResult += "3. Service Status:\n"
                    var serviceStatus = systemInfo.getHawkbitServiceStatus()
                    if (serviceStatus.includes("Active: active")) {
                        diagResult += "✓ PASS - Service running\n\n"
                    } else if (serviceStatus.includes("Active: inactive")) {
                        diagResult += "⚠ INFO - Service stopped\n\n"
                    } else {
                        diagResult += "✗ FAIL - Service error\n\n"
                    }
                    
                    diagResult += "4. Recent Logs:\n"
                    var recentLogs = systemInfo.getHawkbitServiceLogs(5)
                    diagResult += recentLogs.split('\n').slice(-5).join('\n')
                    
                    updateStatus = diagResult
                    updateProgress = 1.0
                    updateComplete = true
                    
                    // Auto-close diagnostic after 10 seconds
                    diagnosticTimer.start()
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
                    systemInfo.logUIEvent("F7 button clicked", "Application exit requested by user")
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
                onClicked: {
                    systemInfo.logUIEvent("F8 button clicked", "System reboot requested by user")
                    systemInfo.rebootSystem()
                }
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
    
    // Diagnostic display timer
    Timer {
        id: diagnosticTimer
        interval: 10000 // 10 seconds
        repeat: false
        onTriggered: {
            if (updateComplete && updateStatus.includes("HAWKBIT DIAGNOSTICS")) {
                systemInfo.logUIEvent("Diagnostic display timeout", "Auto-closing diagnostic popup")
                swUpdateInProgress = false
                updateComplete = false
            }
        }
    }
}