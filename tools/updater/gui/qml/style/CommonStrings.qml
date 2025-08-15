pragma Singleton
import QtQuick 6.0

QtObject {
    id: strings
    
    // Application
    readonly property string appName: qsTr("Updater Server")
    readonly property string appVersion: qsTr("Version 0.2.0")
    readonly property string appDescription: qsTr("OTA Update Management Console")
    
    // General UI
    readonly property string strOk: qsTr("OK")
    readonly property string strCancel: qsTr("Cancel")
    readonly property string strClose: qsTr("Close")
    readonly property string strSave: qsTr("Save")
    readonly property string strDelete: qsTr("Delete")
    readonly property string strEdit: qsTr("Edit")
    readonly property string strRefresh: qsTr("Refresh")
    readonly property string strUpload: qsTr("Upload")
    readonly property string strDownload: qsTr("Download")
    readonly property string strCreate: qsTr("Create")
    readonly property string strUpdate: qsTr("Update")
    readonly property string strSettings: qsTr("Settings")
    readonly property string strAbout: qsTr("About")
    readonly property string strHelp: qsTr("Help")
    readonly property string strLoading: qsTr("Loading...")
    readonly property string strError: qsTr("Error")
    readonly property string strWarning: qsTr("Warning")
    readonly property string strInfo: qsTr("Information")
    readonly property string strSuccess: qsTr("Success")
    
    // Status
    readonly property string statusConnected: qsTr("Connected")
    readonly property string statusDisconnected: qsTr("Disconnected")
    readonly property string statusRunning: qsTr("Running")
    readonly property string statusStopped: qsTr("Stopped")
    readonly property string statusActive: qsTr("Active")
    readonly property string statusInactive: qsTr("Inactive")
    readonly property string statusPending: qsTr("Pending")
    readonly property string statusProcessing: qsTr("Processing")
    readonly property string statusCompleted: qsTr("Completed")
    readonly property string statusFailed: qsTr("Failed")
    
    // Server
    readonly property string serverStatus: qsTr("Server Status")
    readonly property string serverVersion: qsTr("Server Version")
    readonly property string serverProtocol: qsTr("Protocol")
    readonly property string serverUptime: qsTr("Uptime")
    readonly property string serverConnections: qsTr("Active Connections")
    
    // Deployments
    readonly property string deployments: qsTr("Deployments")
    readonly property string deploymentsManagement: qsTr("Deployments Management")
    readonly property string newDeployment: qsTr("New Deployment")
    readonly property string uploadBundle: qsTr("Upload Bundle")
    readonly property string deploymentId: qsTr("Deployment ID")
    readonly property string executionId: qsTr("Execution ID")
    readonly property string version: qsTr("Version")
    readonly property string filename: qsTr("Filename")
    readonly property string fileSize: qsTr("File Size")
    readonly property string description: qsTr("Description")
    readonly property string createdAt: qsTr("Created At")
    readonly property string lastUpdate: qsTr("Last Update")
    readonly property string totalDeployments: qsTr("Total Deployments")
    readonly property string activeDeployments: qsTr("Active Deployments")
    readonly property string noDeployments: qsTr("No deployments available")
    readonly property string noDeploymentsDescription: qsTr("Upload a bundle or create a new deployment to get started")
    
    // Bundles
    readonly property string bundles: qsTr("Bundles")
    readonly property string bundleStorage: qsTr("Bundle Storage")
    readonly property string bundleFiles: qsTr("Bundle Files")
    readonly property string bundleSize: qsTr("Bundle Size")
    readonly property string totalSize: qsTr("Total Size")
    readonly property string selectBundle: qsTr("Select Bundle File")
    readonly property string bundleFormat: qsTr("Bundle Format")
    readonly property string bundleFormatDescription: qsTr("Supported formats: .raucb")
    
    // Activity Log
    readonly property string activityLog: qsTr("Activity Log")
    readonly property string recentActivity: qsTr("Recent Activity")
    readonly property string noActivity: qsTr("No recent activity")
    readonly property string clearLog: qsTr("Clear Log")
    readonly property string exportLog: qsTr("Export Log")
    
    // Actions
    readonly property string actions: qsTr("Actions")
    readonly property string quickActions: qsTr("Quick Actions")
    readonly property string viewDetails: qsTr("View Details")
    readonly property string downloadBundle: qsTr("Download Bundle")
    readonly property string deleteDeployment: qsTr("Delete Deployment")
    readonly property string activateDeployment: qsTr("Activate Deployment")
    readonly property string deactivateDeployment: qsTr("Deactivate Deployment")
    
    // Dialogs
    readonly property string confirmDelete: qsTr("Confirm Delete")
    readonly property string confirmDeleteMessage: qsTr("Are you sure you want to delete this deployment?")
    readonly property string confirmDeleteDescription: qsTr("This action cannot be undone.")
    readonly property string uploadBundleTitle: qsTr("Upload Bundle")
    readonly property string newDeploymentTitle: qsTr("Create New Deployment")
    readonly property string settingsTitle: qsTr("Settings")
    readonly property string aboutTitle: qsTr("About")
    
    // Validation
    readonly property string fieldRequired: qsTr("This field is required")
    readonly property string invalidFormat: qsTr("Invalid format")
    readonly property string fileTooLarge: qsTr("File is too large")
    readonly property string fileNotFound: qsTr("File not found")
    readonly property string invalidVersion: qsTr("Invalid version format")
    readonly property string duplicateId: qsTr("Deployment ID already exists")
    
    // Network
    readonly property string connectionError: qsTr("Connection Error")
    readonly property string serverUnavailable: qsTr("Server Unavailable")
    readonly property string requestTimeout: qsTr("Request Timeout")
    readonly property string networkError: qsTr("Network Error")
    readonly property string retrying: qsTr("Retrying...")
    
    // File Operations
    readonly property string uploadProgress: qsTr("Upload Progress")
    readonly property string uploadCompleted: qsTr("Upload Completed")
    readonly property string uploadFailed: qsTr("Upload Failed")
    readonly property string downloadProgress: qsTr("Download Progress")
    readonly property string downloadCompleted: qsTr("Download Completed")
    readonly property string downloadFailed: qsTr("Download Failed")
    
    // Security
    readonly property string httpsEnabled: qsTr("HTTPS Enabled")
    readonly property string httpMode: qsTr("HTTP Mode")
    readonly property string secureConnection: qsTr("Secure Connection")
    readonly property string insecureConnection: qsTr("Insecure Connection")
    readonly property string certificateValid: qsTr("Certificate Valid")
    readonly property string certificateInvalid: qsTr("Certificate Invalid")
}