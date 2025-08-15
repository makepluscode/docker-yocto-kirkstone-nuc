import QtQuick 2.15
import RaucSystem 1.0
import UpdateAgent 1.0

DashboardCardBase {
    title: "Update Agent"
    property RaucSystemManager raucSystemManager: null
    property UpdateAgentManager updateAgentManager: null

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        // Service status indicator
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: {
                    if (updateAgentManager) {
                        if (updateAgentManager.isUpdateActive) return "#4a9eff" // Blue for active update
                        if (updateAgentManager.isServiceRunning) return "#44ff44" // Green for running
                        return "#ff4444" // Red for not running
                    }
                    return "#ffff44" // Yellow for unknown
                }
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Update Agent"
                color: "#ffffff"
                font.pointSize: 10
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // Status information
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 6

            CardInfoRow {
                label: "Service"
                value: "update-agent.service"
                labelWidth: 60
                valueColor: "#cccccc"
            }

            CardInfoRow {
                label: "Status"
                value: updateAgentManager ? (updateAgentManager.isUpdateActive ? updateAgentManager.updateStatus : (updateAgentManager.isServiceRunning ? "Running" : "Stopped")) : "Unknown"
                labelWidth: 60
                valueColor: {
                    if (!updateAgentManager) return "#888888"
                    if (updateAgentManager.isUpdateActive) return "#4a9eff"
                    return updateAgentManager.isServiceRunning ? "#44ff44" : "#ff4444"
                }
            }

            CardInfoRow {
                label: "Progress"
                value: updateAgentManager && updateAgentManager.isUpdateActive ? updateAgentManager.updateProgress + "%" : "—"
                labelWidth: 60
                valueColor: "#ffff44"
            }

            CardInfoRow {
                label: "Controller"
                value: "nuc-device-001"
                labelWidth: 60
                valueColor: "#888888"
            }
        }

        // Info message
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: updateAgentManager && updateAgentManager.isUpdateActive ? "Update in progress..." : "OTA updates via Hawkbit"
            color: updateAgentManager && updateAgentManager.isUpdateActive ? "#4a9eff" : "#888888"
            font.pointSize: 9
            horizontalAlignment: Text.AlignHCenter
        }
    }
}