import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 720
    minimumWidth: 1280
    maximumWidth: 1280
    minimumHeight: 720
    maximumHeight: 720
    title: "Updater - OTA Update Management"
    
    // Theme colors
    property color primaryColor: "#2c3e50"
    property color secondaryColor: "#34495e"
    property color accentColor: "#3498db"
    property color successColor: "#27ae60"
    property color warningColor: "#f39c12"
    property color errorColor: "#e74c3c"
    property color backgroundColor: "#ecf0f1"
    property color textColor: "#2c3e50"
    
    // Application state
    property bool serverRunning: false
    property string serverStatus: "Stopped"
    property var deployments: []
    property var logs: []
    property var bundles: []
    property var latestBundle: null
    
    // Timer for automatic bundle refresh
    Timer {
        id: autoRefreshTimer
        interval: 5000 // 5 seconds
        running: true
        repeat: true
        onTriggered: {
            scanBundles()
        }
    }
    
    // Bundle selection handler
    function handleBundleSelection() {
        addLog("📁 Select Bundle button clicked")
        addLog("ℹ️ File dialog feature not available in this QML version")
        addLog("ℹ️ Please manually copy bundle files to the 'bundle/' directory")
        addLog("ℹ️ The latest bundle will be automatically detected")
    }
    
    // Component imports
    Component.onCompleted: {
        // Initialize the application
        console.log("Updater application started")
        addLog("🚀 Updater application started")
        addLog("🔍 Initializing bundle scanner...")
        scanBundles()
        addLog("✅ Application initialization complete")
        addLog("🔄 Auto-refresh enabled (every 5 seconds)")
    }
    
    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: primaryColor
            radius: 8
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                
                Text {
                    text: "🔄 Updater"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
                
                // Server status indicator
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: serverRunning ? successColor : errorColor
                    
                    SequentialAnimation on opacity {
                        running: serverRunning
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.5; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }
                
                Text {
                    text: serverStatus
                    color: "white"
                    font.pixelSize: 14
                }
                
                // Server control buttons
                Button {
                    text: serverRunning ? "Stop Server" : "Start Server"
                    background: Rectangle {
                        color: serverRunning ? errorColor : successColor
                        radius: 4
                    }
                    onClicked: {
                        if (serverRunning) {
                            stopServer()
                        } else {
                            startServer()
                        }
                    }
                }
            }
        }
        
        // Top panel - Bundle Info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            color: "white"
            radius: 8
            border.color: "#bdc3c7"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                Text {
                    text: "📦 Latest Bundle"
                    font.pixelSize: 18
                    font.bold: true
                    color: textColor
                }
                
                // Latest bundle display
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#e3f2fd"
                    radius: 6
                    border.color: "#2196f3"
                    border.width: 2
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 8
                        
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: "#2196f3"
                            }
                            
                            Text {
                                text: "⭐ Latest Bundle Selected"
                                font.pixelSize: 16
                                font.bold: true
                                color: "#2196f3"
                            }
                        }
                        
                        Text {
                            text: "Name: " + (latestBundle ? latestBundle.name : "No bundle available")
                            font.pixelSize: 14
                            color: "#2c3e50"
                            elide: Text.ElideRight
                        }
                        
                        Text {
                            text: "Version: " + (latestBundle ? latestBundle.version : "Unknown")
                            font.pixelSize: 14
                            color: "#2c3e50"
                        }
                        
                        Text {
                            text: "Size: " + (latestBundle ? latestBundle.size_mb + " MB" : "0 MB") + " | Created: " + (latestBundle ? latestBundle.mtime_str : "Unknown")
                            font.pixelSize: 14
                            color: "#2c3e50"
                            elide: Text.ElideRight
                        }
                        
                        Item { Layout.fillHeight: true }
                        
                        // Select Bundle button
                        Button {
                            Layout.fillWidth: true
                            text: "📁 Select Bundle"
                            background: Rectangle {
                                color: "#3498db"
                                radius: 4
                            }
                            onClicked: {
                                handleBundleSelection()
                            }
                        }
                    }
                }
            }
        }
        
        // Bottom panel - Server Log Viewer
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            radius: 8
            border.color: "#bdc3c7"
            border.width: 1
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                Text {
                    text: "📋 Activity Logs"
                    font.pixelSize: 18
                    font.bold: true
                    color: textColor
                }
                
                // Log display
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    TextArea {
                        id: logArea
                        width: parent.width
                        height: parent.height
                        text: logs.join('\n')
                        readOnly: true
                        font.family: "Monospace"
                        font.pixelSize: 12
                        background: Rectangle {
                            color: backgroundColor
                            radius: 4
                        }
                        
                        // Auto-scroll to bottom when logs change
                        onTextChanged: {
                            if (text.length > 0) {
                                cursorPosition = text.length
                            }
                        }
                    }
                }
                
                // Log controls
                RowLayout {
                    Layout.fillWidth: true
                    
                    Button {
                        text: "🗑️ Clear Logs"
                        background: Rectangle {
                            color: warningColor
                            radius: 4
                        }
                        onClicked: {
                            clearLogs()
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    CheckBox {
                        text: "Auto-scroll"
                        checked: true
                    }
                }
            }
        }
    }
    
    // Add deployment dialog
    Dialog {
        id: addDeploymentDialog
        title: "Add New Deployment"
        width: 500
        height: 400
        modal: true
        
        property string name: ""
        property string bundleFile: ""
        property string version: ""
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 15
            
            Label {
                text: "Deployment Name:"
                font.pixelSize: 14
                font.bold: true
            }
            
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "Enter deployment name"
                onTextChanged: addDeploymentDialog.name = text
            }
            
            Label {
                text: "Bundle File:"
                font.pixelSize: 14
                font.bold: true
            }
            
            TextField {
                id: bundleField
                Layout.fillWidth: true
                placeholderText: "Enter bundle filename (e.g., my-bundle.raucb)"
                onTextChanged: addDeploymentDialog.bundleFile = text
            }
            
            Label {
                text: "Version:"
                font.pixelSize: 14
                font.bold: true
            }
            
            TextField {
                id: versionField
                Layout.fillWidth: true
                placeholderText: "Enter version (e.g., 1.0.0)"
                onTextChanged: addDeploymentDialog.version = text
            }
            
            Item { Layout.fillHeight: true }
            
            RowLayout {
                Layout.fillWidth: true
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "Cancel"
                    onClicked: addDeploymentDialog.close()
                }
                
                Button {
                    text: "Add Deployment"
                    enabled: nameField.text.trim() !== "" && bundleField.text !== "" && versionField.text.trim() !== ""
                    background: Rectangle {
                        color: parent.enabled ? "#3498db" : "#bdc3c7"
                        radius: 4
                    }
                    onClicked: {
                        addDeployment(nameField.text.trim(), bundleField.text, versionField.text.trim())
                        addDeploymentDialog.close()
                        
                        // Reset fields
                        nameField.text = ""
                        bundleField.text = ""
                        versionField.text = ""
                    }
                }
            }
        }
    }
    
    // Functions
    function startServer() {
        addLog("🚀 Starting server...")
        if (serverManager) {
            serverManager.startServer()
        } else {
            serverRunning = true
            serverStatus = "Running"
            addLog("✅ Server started successfully")
        }
    }
    
    function stopServer() {
        addLog("🛑 Stopping server...")
        if (serverManager) {
            serverManager.stopServer()
        } else {
            serverRunning = false
            serverStatus = "Stopped"
            addLog("🛑 Server stopped")
        }
    }
    
    function addLog(message) {
        // Send to Python backend if available
        if (logManager) {
            logManager.addLog(message)
        } else {
            // Fallback to local logging
            var timestamp = new Date().toLocaleTimeString()
            var logEntry = "[" + timestamp + "] " + message
            logs.push(logEntry)
            
            // Keep only last 100 logs
            if (logs.length > 100) {
                logs = logs.slice(-100)
            }
            
            // Update log area
            logArea.text = logs.join('\n')
        }
    }
    
    function clearLogs() {
        // Clear Python backend if available
        if (logManager) {
            logManager.clearLogs()
        } else {
            // Fallback to local clearing
            logs = []
            logArea.text = ""
        }
    }
    
    function toggleDeployment(id, active) {
        if (deploymentManager) {
            deploymentManager.toggleDeployment(id, active)
        } else {
            addLog("🔄 Deployment " + id + " " + (active ? "enabled" : "disabled"))
        }
    }
    
    function scanBundles() {
        addLog("🔍 Scanning bundle directory...")
        
        // Use Python backend if available, otherwise use mock data
        if (deploymentManager) {
            addLog("📡 Using Python backend for bundle scanning...")
            deploymentManager.scanBundles()
        } else {
            addLog("⚠️ Using fallback mock data...")
            // Fallback to mock data
            var mockBundles = [
                {
                    name: "nuc-image-qt5-bundle-intel-corei7-64-20250816051855.raucb",
                    version: "2025.08.16-051855",
                    size_mb: 152.0,
                    mtime_str: "2025-08-16 05:18:55"
                },
                {
                    name: "nuc-image-qt5-bundle-intel-corei7-64-20250816044423.raucb",
                    version: "2025.08.16-044423",
                    size_mb: 152.0,
                    mtime_str: "2025-08-16 04:44:23"
                },
                {
                    name: "nuc-image-qt5-bundle-intel-corei7-64-20250816043813.raucb",
                    version: "2025.08.16-043813",
                    size_mb: 152.0,
                    mtime_str: "2025-08-16 04:38:13"
                }
            ]
            
            bundles = mockBundles
            latestBundle = mockBundles[0]  // First one is latest
            
            addLog("✅ Found " + bundles.length + " bundle files")
            addLog("⭐ Latest bundle: " + latestBundle.name)
        }
    }
    
    function deployLatestBundle() {
        if (latestBundle) {
            addLog("🚀 Deploying latest bundle: " + latestBundle.name)
            addLog("📦 Version: " + latestBundle.version)
            addLog("📏 Size: " + latestBundle.size_mb + " MB")
            
            // Use Python backend if available
            if (deploymentManager) {
                deploymentManager.createDeploymentFromLatest("Deployment-" + latestBundle.version)
            } else {
                // Fallback to mock deployment
                var deployment = {
                    name: "Deployment-" + latestBundle.version,
                    version: latestBundle.version,
                    filename: latestBundle.name,
                    active: true
                }
                
                deployments.push(deployment)
                addLog("✅ Deployment created: " + deployment.name)
            }
        } else {
            addLog("❌ No latest bundle available")
        }
    }
    
    function addDeployment(name, bundleFile, version) {
        if (deploymentManager) {
            deploymentManager.addDeployment(name, bundleFile, version)
        } else {
            addLog("✅ Added deployment: " + name)
        }
    }
} 